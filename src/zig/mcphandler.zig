const std = @import("std");
const net = std.net;
const Allocator = std.mem.Allocator;
const database = @import("database/sqlitehandler.zig");

const NVIDIA_API_URL = "https://integrate.api.nvidia.com/v1/chat/completions";
const NVIDIA_MODEL = "nvidia/nemotron-3-nano-30b-a3b";

/// MCP Handler for NVIDIA AI integration
pub const MCPHandler = struct {
    const Self = @This();

    allocator: Allocator,
    api_token: []const u8,
    db: *database.SqliteHandler,

    pub fn init(allocator: Allocator, db: *database.SqliteHandler, token: []const u8) Self {
        return Self{
            .allocator = allocator,
            .api_token = token,
            .db = db,
        };
    }

    pub fn deinit(self: *Self) void {
        _ = self;
        std.debug.print("[MCPHandler] Deinit\n", .{});
    }

    /// Process an analysis request and send response through WebSocket
    pub fn processRequest(
        self: *Self,
        allocator: Allocator,
        stream: net.Stream,
        id: []const u8,
        request_type: []const u8,
        file_path: []const u8,
        content: ?[]const u8,
        user_prompt: ?[]const u8,
        isStream: ?bool,
    ) !void {
        std.debug.print("[MCPHandler] Processing {s} request for path: {s}\n and content {s}", .{ request_type, file_path, content orelse "none" });

        // Get file content if path is provided and content is not
        var file_content: []const u8 = undefined;
        var should_free_content = false;

        // if (content) |c| {
        //     file_content = c;
        // } else if (file_path) |path| {
        std.debug.print("File path not null {s}", .{file_path});
        file_content = self.readFileOrFolder(allocator, file_path) catch |err| {
            try self.sendError(stream, id, "Failed to read file/folder", err);
            return;
        };
        should_free_content = true;
        // } else {
        //     try self.sendError(stream, id, "No file path or content provided", error.MissingInput);
        //     return;
        // }
        defer if (should_free_content) allocator.free(file_content);

        std.debug.print("[MCPHandler] Request body length: {d}\n", .{file_content.len});
        std.debug.print("[MCPHandler] {s}\n", .{file_content});
        // Build the AI prompt
        const prompt = try self.buildPrompt(allocator, request_type, file_path, file_content, user_prompt);

        defer allocator.free(prompt);

        // Send status update
        // try self.sendStatus(stream, id, "processing", "Sending request to NVIDIA AI...");

        // Call NVIDIA API
        self.callNvidiaAPI(allocator, stream, id, prompt, isStream) catch |err| {
            try self.sendError(stream, id, "NVIDIA API call failed", err);
            return;
        };
    }

    /// Read file or folder contents
    fn readFileOrFolder(self: *Self, allocator: Allocator, path: []const u8) ![]const u8 {
        _ = self;

        // Convert to null-terminated path for std.fs
        const path_z = try allocator.dupeZ(u8, path);
        defer allocator.free(path_z);

        // Try to open as file first
        const file = std.fs.cwd().openFile(path_z, .{}) catch |err| {
            if (err == error.IsDir) {
                // It's a directory, read all files
                return try readDirectoryContents(allocator, path_z);
            }
            return err;
        };
        defer file.close();

        // Read file content
        const stat = try file.stat();
        const max_size: usize = 1024 * 1024; // 1MB max
        const size = @min(stat.size, max_size);

        const content = try allocator.alloc(u8, size);
        errdefer allocator.free(content);

        const bytes_read = try file.readAll(content);
        if (bytes_read < size) {
            return allocator.realloc(content, bytes_read) catch content[0..bytes_read];
        }

        return content;
    }

    /// Build AI prompt based on request type
    fn buildPrompt(
        self: *Self,
        allocator: Allocator,
        request_type: []const u8,
        file_path: ?[]const u8,
        content: []const u8,
        user_prompt: ?[]const u8,
    ) ![]const u8 {
        _ = self;

        const system_prompt = if (std.mem.eql(u8, request_type, "analyze"))
            "You are an expert code analyst. Analyze the following code and provide insights about its structure, patterns, and potential issues."
        else if (std.mem.eql(u8, request_type, "explain"))
            "You are a helpful programming teacher. Explain the following code in detail, including what each part does and why."
        else if (std.mem.eql(u8, request_type, "review"))
            "You are a senior code reviewer. Review the following code and provide constructive feedback on code quality, best practices, potential bugs, and improvements."
        else if (std.mem.eql(u8, request_type, "refactor"))
            "You are an expert software engineer. Suggest refactoring improvements for the following code to make it cleaner, more efficient, and maintainable."
        else
            "You are a helpful AI assistant. Analyze and respond to the following content.";

        var prompt_builder: std.ArrayList(u8) = .empty;
        errdefer prompt_builder.deinit(allocator);

        try prompt_builder.appendSlice(allocator, system_prompt);
        try prompt_builder.appendSlice(allocator, "\n\n");

        if (file_path) |path| {
            try prompt_builder.appendSlice(allocator, "File: ");
            try prompt_builder.appendSlice(allocator, path);
            try prompt_builder.appendSlice(allocator, "\n\n");
        }

        try prompt_builder.appendSlice(allocator, "```\n");
        try prompt_builder.appendSlice(allocator, content);
        try prompt_builder.appendSlice(allocator, "\n```\n");

        if (user_prompt) |up| {
            try prompt_builder.appendSlice(allocator, "\nAdditional instructions: ");
            try prompt_builder.appendSlice(allocator, up);
        }

        return try prompt_builder.toOwnedSlice(allocator);
    }

    /// Call NVIDIA API and stream response through WebSocket
    fn callNvidiaAPI(
        self: *Self,
        allocator: Allocator,
        stream: net.Stream,
        request_id: []const u8,
        prompt: []const u8,
        isStream: ?bool,
    ) !void {
        // Build request body
        const use_stream = isStream orelse false;
        const request_body = try self.buildRequestBody(allocator, prompt, use_stream);
        defer allocator.free(request_body);

        // Handle streaming mode
        if (use_stream) {
            std.debug.print("[MCPHandler] Streaming request body length: {d}\n", .{request_body.len});
            var responseMessage = try std.ArrayList(u8).initCapacity(allocator, request_body.len + 256);
            defer responseMessage.deinit(allocator); // free when we’re done
            // Build authorization header
            const auth_header = try std.fmt.allocPrint(allocator, "Bearer {s}", .{self.api_token});
            defer allocator.free(auth_header);

            // Create HTTP client
            var client = std.http.Client{ .allocator = allocator };
            defer client.deinit();

            // Set up extra headers for SSE streaming
            const extra_headers: []const std.http.Header = &.{
                .{ .name = "Content-Type", .value = "application/json" },
                .{ .name = "Accept", .value = "text/event-stream" },
                .{ .name = "Authorization", .value = auth_header },
            };

            // Create an allocating writer to capture the SSE response
            var response_writer_alloc: std.Io.Writer.Allocating = .init(allocator);
            defer response_writer_alloc.deinit();

            // Perform the request using fetch
            const result = client.fetch(.{
                .location = .{ .url = NVIDIA_API_URL },
                .method = .POST,
                .payload = request_body,
                .extra_headers = extra_headers,
                .response_writer = &response_writer_alloc.writer,
            }) catch |err| {
                std.debug.print("[MCPHandler] Streaming fetch failed: {}\n", .{err});
                try self.sendStatus(stream, request_id, "error", "Failed to connect to NVIDIA API");
                return;
            };

            // Check response status
            if (result.status != .ok) {
                std.debug.print("[MCPHandler] Streaming API returned status: {}\n", .{result.status});
                try self.sendStatus(stream, request_id, "error", "NVIDIA API returned error");
                return;
            }

            // Get the full SSE response and process line by line
            const sse_body = response_writer_alloc.written();
            std.debug.print("[MCPHandler] SSE response length: {d}\n", .{sse_body.len});

            // Process SSE data lines
            var lines = std.mem.splitScalar(u8, sse_body, '\n');
            while (lines.next()) |line| {
                const trimmed = std.mem.trim(u8, line, " \r\n");

                // Check for SSE data prefix
                if (std.mem.startsWith(u8, trimmed, "data: ")) {
                    const data = trimmed[6..];

                    // Skip [DONE] marker
                    if (std.mem.eql(u8, data, "[DONE]")) {
                        continue;
                    }

                    // Parse JSON chunk
                    const parsed = std.json.parseFromSlice(std.json.Value, allocator, data, .{}) catch continue;
                    defer parsed.deinit();

                    // Extract content from choices[0].delta.content
                    if (parsed.value.object.get("choices")) |choices| {
                        if (choices.array.items.len > 0) {
                            if (choices.array.items[0].object.get("delta")) |delta| {
                                if (delta.object.get("content")) |content_val| {
                                    const content_chunk = content_val.string;
                                    // Send chunk to WebSocket
                                    try responseMessage.appendSlice(allocator, content_chunk);
                                    try self.sendChunk(stream, request_id, content_chunk);
                                }
                            }
                        }
                    }
                }
            }

            // Send completion message
            const finalizeResponse: []const u8 = try responseMessage.toOwnedSlice(allocator);
            self.db.insertHistoryChat(finalizeResponse, "", "assistant", "") catch |err| {
                std.debug.print("[MCPHandler] Failed to save history: {}\n", .{err});
            };
            try self.sendStatus(stream, request_id, "complete", "");
            return;
        }

        std.debug.print("[MCPHandler] Request body length: {d}\n", .{request_body.len});
        std.debug.print("[MCPHandler] {s}\n", .{request_body});

        // Build authorization header
        const auth_header = try std.fmt.allocPrint(allocator, "Bearer {s}", .{self.api_token});
        defer allocator.free(auth_header);

        // Create HTTP client
        var client = std.http.Client{ .allocator = allocator };
        defer client.deinit();

        // Set up extra headers
        const extra_headers: []const std.http.Header = &.{
            .{ .name = "Content-Type", .value = "application/json" },
            .{ .name = "Accept", .value = "application/json" },
            .{ .name = "Authorization", .value = auth_header },
        };

        // Create an allocating writer to capture the body
        var response_writer_alloc: std.Io.Writer.Allocating = .init(allocator);
        defer response_writer_alloc.deinit();

        // Perform the request using fetch
        const result = client.fetch(.{
            .location = .{ .url = NVIDIA_API_URL },
            .method = .POST,
            .payload = request_body,
            .extra_headers = extra_headers,
            .response_writer = &response_writer_alloc.writer,
        }) catch |err| {
            std.debug.print("[MCPHandler] Fetch request failed: {}\n", .{err});
            try self.sendStatus(stream, request_id, "error", "Failed to connect to NVIDIA API");
            return;
        };

        // Check response status
        if (result.status != .ok) {
            std.debug.print("[MCPHandler] API returned status: {}\n", .{result.status});
            std.debug.print("[MCPHandler] Response: {s}\n", .{response_writer_alloc.written()});
            try self.sendStatus(stream, request_id, "error", "NVIDIA API returned error");
            return;
        }

        // Get response body
        const body = response_writer_alloc.written();

        std.debug.print("[MCPHandler] Response received, length: {d}\n", .{body.len});

        // Parse JSON to extract content for database
        if (std.json.parseFromSlice(std.json.Value, self.allocator, body, .{})) |parsed| {
            defer parsed.deinit();
            const root = parsed.value.object;
            if (root.get("choices")) |choices| {
                if (choices.array.items.len > 0) {
                    if (choices.array.items[0].object.get("message")) |message| {
                        if (message.object.get("content")) |content_val| {
                            const content = content_val.string;

                            self.db.insertHistoryChat(content, "", "assistant", "") catch |err| {
                                std.debug.print("[MCPHandler] Failed to save history: {}\n", .{err});
                            };
                        }
                    }
                }
            }
        } else |err| {
            std.debug.print("[MCPHandler] JSON parse error for history: {}\n", .{err});
        }

        // Process and send response to WebSocket
        try self.processResponse(allocator, stream, request_id, body);
    }

    /// Build NVIDIA API request body
    fn buildRequestBody(self: *Self, allocator: Allocator, prompt: []const u8, isStream: ?bool) ![]const u8 {
        _ = self;

        var body: std.ArrayList(u8) = .empty;
        errdefer body.deinit(allocator);

        try body.appendSlice(allocator, "{\"model\":\"");
        try body.appendSlice(allocator, NVIDIA_MODEL);
        try body.appendSlice(allocator, "\",\"messages\":[{\"role\":\"user\",\"content\":\"");

        // Escape prompt for JSON
        for (prompt) |ch| {
            switch (ch) {
                '"' => try body.appendSlice(allocator, "\\\""),
                '\\' => try body.appendSlice(allocator, "\\\\"),
                '\n' => try body.appendSlice(allocator, "\\n"),
                '\r' => try body.appendSlice(allocator, "\\r"),
                '\t' => try body.appendSlice(allocator, "\\t"),
                else => {
                    if (ch < 0x20) {
                        try body.appendSlice(allocator, "\\u00");
                        const hex = "0123456789abcdef";
                        try body.append(allocator, hex[ch >> 4]);
                        try body.append(allocator, hex[ch & 0x0F]);
                    } else {
                        try body.append(allocator, ch);
                    }
                },
            }
        }
        const stream = if (isStream orelse false) "true" else "false";
        try body.appendSlice(allocator, "\"}],\"temperature\":0.7,\"top_p\":1,\"max_tokens\":16384,\"stream\":");
        try body.appendSlice(allocator, stream);
        // try body.appendSlice(allocator, ",\"chat_template_kwargs\":{");
        // try body.appendSlice(allocator, "\"enable_thinking\":true}");
        try body.appendSlice(allocator, ",\"reasoning_effort\":");
        try body.appendSlice(allocator, "\"high\"");
        // try body.appendSlice(allocator, ",\"frequent_penalty\":");
        // try body.appendSlice(allocator, "\"0.00\"");
        // try body.appendSlice(allocator, ",\"presence_penalty\":");
        // try body.appendSlice(allocator, "\"0.00\"");
        try body.appendSlice(allocator, "}");

        return try body.toOwnedSlice(allocator);
    }

    /// Process SSE streaming response from NVIDIA API
    fn processStreamingResponse(
        self: *Self,
        allocator: Allocator,
        stream: net.Stream,
        request_id: []const u8,
        reader: anytype,
    ) !void {
        var line_buf: [8192]u8 = undefined;
        var content_accumulator: std.ArrayList(u8) = .empty;
        defer content_accumulator.deinit(allocator);

        while (true) {
            const line = reader.readUntilDelimiterOrEof(&line_buf, '\n') catch |err| {
                std.debug.print("[MCPHandler] Read error: {}\n", .{err});
                break;
            };

            if (line == null) break;

            const trimmed = std.mem.trim(u8, line.?, " \r\n");

            // Skip empty lines
            if (trimmed.len == 0) continue;

            // Check for SSE data prefix
            if (std.mem.startsWith(u8, trimmed, "data: ")) {
                const data = trimmed[6..];

                // Check for stream end
                if (std.mem.eql(u8, data, "[DONE]")) {
                    std.debug.print("[MCPHandler] Stream complete\n", .{});
                    break;
                }

                // Parse JSON chunk
                const parsed = std.json.parseFromSlice(std.json.Value, allocator, data, .{}) catch {
                    continue; // Skip malformed chunks
                };
                defer parsed.deinit();

                // Extract content from choices[0].delta.content
                if (parsed.value.object.get("choices")) |choices| {
                    if (choices.array.items.len > 0) {
                        if (choices.array.items[0].object.get("delta")) |delta| {
                            if (delta.object.get("content")) |content_val| {
                                const content_chunk = content_val.string;
                                try content_accumulator.appendSlice(allocator, content_chunk);

                                // Send chunk to WebSocket
                                try self.sendChunk(stream, request_id, content_chunk);
                            }
                        }
                    }
                }
            }
        }

        // Send completion message
        try self.sendStatus(stream, request_id, "complete", "");
    }

    /// Process non-streaming response from NVIDIA API
    fn processResponse(
        self: *Self,
        allocator: Allocator,
        stream: net.Stream,
        request_id: []const u8,
        response_body: []const u8,
    ) !void {
        _ = allocator;

        // Parse JSON response
        var parser = std.json.parseFromSlice(std.json.Value, self.allocator, response_body, .{}) catch |err| {
            std.debug.print("[MCPHandler] Failed to parse response: {}\n", .{err});
            std.debug.print("[MCPHandler] Response body: {s}\n", .{response_body[0..@min(response_body.len, 500)]});
            try self.sendErrorResponse(stream, request_id, "Failed to parse API response");
            return;
        };
        defer parser.deinit();

        // Extract content from choices[0].message.content
        const root = parser.value.object;

        if (root.get("choices")) |choices| {
            if (choices.array.items.len > 0) {
                if (choices.array.items[0].object.get("message")) |message| {
                    if (message.object.get("content")) |content_val| {
                        const content = content_val.string;

                        // Send the complete response wrapped in success format
                        try self.sendSuccessResponse(stream, request_id, content);
                        return;
                    }
                }
            }
        }

        // Check for error in response
        if (root.get("error")) |error_obj| {
            if (error_obj.object.get("message")) |msg| {
                try self.sendErrorResponse(stream, request_id, msg.string);
                return;
            }
        }

        try self.sendErrorResponse(stream, request_id, "Unexpected API response format");
    }

    /// Send success response with content
    fn sendSuccessResponse(self: *Self, stream: net.Stream, id: []const u8, content: []const u8) !void {
        // Use dynamic buffer for potentially large content
        var json_builder = try std.ArrayList(u8).initCapacity(self.allocator, content.len + 256);
        defer json_builder.deinit(self.allocator);

        try json_builder.appendSlice(self.allocator, "{\"id\":\"");
        try json_builder.appendSlice(self.allocator, id);
        try json_builder.appendSlice(self.allocator, "\",\"success\":true,\"content\":\"");

        // Escape content for JSON
        for (content) |ch| {
            switch (ch) {
                '"' => try json_builder.appendSlice(self.allocator, "\\\""),
                '\\' => try json_builder.appendSlice(self.allocator, "\\\\"),
                '\n' => try json_builder.appendSlice(self.allocator, "\\n"),
                '\r' => try json_builder.appendSlice(self.allocator, "\\r"),
                '\t' => try json_builder.appendSlice(self.allocator, "\\t"),
                else => {
                    if (ch < 0x20) {
                        // Skip control characters
                    } else {
                        try json_builder.append(self.allocator, ch);
                    }
                },
            }
        }

        try json_builder.appendSlice(self.allocator, "\"}");

        try self.sendWebSocketFrame(stream, json_builder.items);
    }

    /// Send error response
    fn sendErrorResponse(self: *Self, stream: net.Stream, id: []const u8, error_msg: []const u8) !void {
        var json_buf: [4096]u8 = undefined;
        var fbs = std.io.fixedBufferStream(&json_buf);
        const writer = fbs.writer();

        try writer.writeAll("{\"id\":\"");
        try writer.writeAll(id);
        try writer.writeAll("\",\"success\":false,\"error\":\"");
        try writer.writeAll(error_msg);
        try writer.writeAll("\"}");

        try self.sendWebSocketFrame(stream, fbs.getWritten());
    }

    /// Send a content chunk through WebSocket
    fn sendChunk(self: *Self, stream: net.Stream, id: []const u8, content: []const u8) !void {
        var json_buf: [8192]u8 = undefined;
        var fbs = std.io.fixedBufferStream(&json_buf);
        const writer = fbs.writer();

        try writer.writeAll("{\"id\":\"");
        try writer.writeAll(id);
        try writer.writeAll("\",\"status\":\"streaming\",\"content\":\"");

        // Escape content
        for (content) |ch| {
            switch (ch) {
                '"' => try writer.writeAll("\\\""),
                '\\' => try writer.writeAll("\\\\"),
                '\n' => try writer.writeAll("\\n"),
                '\r' => try writer.writeAll("\\r"),
                '\t' => try writer.writeAll("\\t"),
                else => try writer.writeByte(ch),
            }
        }

        try writer.writeAll("\"}");

        try self.sendWebSocketFrame(stream, fbs.getWritten());
    }

    /// Send status message through WebSocket
    fn sendStatus(self: *Self, stream: net.Stream, id: []const u8, status: []const u8, message: []const u8) !void {
        var json_buf: [4096]u8 = undefined;
        var fbs = std.io.fixedBufferStream(&json_buf);
        const writer = fbs.writer();

        try writer.writeAll("{\"id\":\"");
        try writer.writeAll(id);
        try writer.writeAll("\",\"status\":\"");
        try writer.writeAll(status);
        try writer.writeAll("\",\"content\":\"");

        // Escape message
        for (message) |ch| {
            switch (ch) {
                '"' => try writer.writeAll("\\\""),
                '\\' => try writer.writeAll("\\\\"),
                '\n' => try writer.writeAll("\\n"),
                '\r' => try writer.writeAll("\\r"),
                '\t' => try writer.writeAll("\\t"),
                else => try writer.writeByte(ch),
            }
        }

        try writer.writeAll("\"}");

        try self.sendWebSocketFrame(stream, fbs.getWritten());
    }

    /// Send error message through WebSocket
    fn sendError(self: *Self, stream: net.Stream, id: []const u8, message: []const u8, err: anyerror) !void {
        var error_msg_buf: [512]u8 = undefined;
        const error_msg = std.fmt.bufPrint(&error_msg_buf, "{s}: {}", .{ message, err }) catch message;
        try self.sendStatus(stream, id, "error", error_msg);
    }

    /// Send WebSocket text frame
    fn sendWebSocketFrame(self: *Self, stream: net.Stream, payload: []const u8) !void {
        _ = self;

        var header: [10]u8 = undefined;
        var header_len: usize = 2;

        // FIN bit + text opcode
        header[0] = 0x81;

        // Payload length
        if (payload.len < 126) {
            header[1] = @intCast(payload.len);
        } else if (payload.len < 65536) {
            header[1] = 126;
            header[2] = @intCast((payload.len >> 8) & 0xFF);
            header[3] = @intCast(payload.len & 0xFF);
            header_len = 4;
        } else {
            header[1] = 127;
            var len = payload.len;
            for (0..8) |i| {
                header[9 - i] = @intCast(len & 0xFF);
                len >>= 8;
            }
            header_len = 10;
        }

        _ = try stream.writeAll(header[0..header_len]);
        _ = try stream.writeAll(payload);
    }
};

/// Read all files from a directory recursively
fn readDirectoryContents(allocator: Allocator, path: [:0]const u8) ![]const u8 {
    var content: std.ArrayList(u8) = .empty;
    errdefer content.deinit(allocator);

    var dir = try std.fs.cwd().openDir(path, .{ .iterate = true });
    defer dir.close();

    var walker = try dir.walk(allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        // Only process files
        if (entry.kind != .file) continue;

        // Skip hidden files and common binary files
        if (entry.basename[0] == '.') continue;
        if (isLikelyBinary(entry.basename)) continue;

        // Build full path
        try content.appendSlice(allocator, "\n--- File: ");
        try content.appendSlice(allocator, entry.path);
        try content.appendSlice(allocator, " ---\n");

        // Read file content
        const file = dir.openFile(entry.path, .{}) catch continue;
        defer file.close();

        const stat = file.stat() catch continue;
        const max_file_size: usize = 64 * 1024; // 64KB per file max

        if (stat.size > max_file_size) {
            try content.appendSlice(allocator, "[File too large, truncated]\n");
        }

        const size: usize = @min(stat.size, max_file_size);
        const file_buf = try allocator.alloc(u8, size);
        defer allocator.free(file_buf);

        const bytes_read = file.readAll(file_buf) catch continue;
        try content.appendSlice(allocator, file_buf[0..bytes_read]);
        try content.appendSlice(allocator, "\n");
    }

    return try content.toOwnedSlice(allocator);
}

/// Check if file is likely binary based on extension
fn isLikelyBinary(basename: []const u8) bool {
    const binary_extensions = [_][]const u8{
        ".exe", ".dll",  ".so",   ".dylib", ".bin", ".obj",  ".o",
        ".zip", ".tar",  ".gz",   ".rar",   ".7z",  ".db",   ".sqlite",
        ".png", ".jpg",  ".jpeg", ".gif",   ".bmp", ".ico",  ".webp",
        ".mp3", ".mp4",  ".avi",  ".mov",   ".wav", ".flac", ".pdf",
        ".doc", ".docx", ".xls",  ".xlsx",  ".ppt", ".pptx",
    };

    for (binary_extensions) |ext| {
        if (std.mem.endsWith(u8, basename, ext)) return true;
    }
    return false;
}

/// Load NVIDIA API token from .env file
pub fn loadNvidiaToken(allocator: Allocator, path: []const u8) ![]const u8 {
    const path_z = try allocator.dupeZ(u8, path);
    defer allocator.free(path_z);

    const file = try std.fs.cwd().openFile(path_z, .{});
    defer file.close();

    const content = try file.readToEndAlloc(allocator, 4096);
    defer allocator.free(content);

    // Find NVIDIA_API_KEY=
    var lines = std.mem.splitScalar(u8, content, '\n');
    while (lines.next()) |line| {
        const trimmed = std.mem.trim(u8, line, " \r");
        if (std.mem.startsWith(u8, trimmed, "NVIDIA_API_KEY=")) {
            const token = std.mem.trim(u8, trimmed[15..], " \r\n\"'");
            return try allocator.dupe(u8, token);
        }
    }

    return error.TokenNotFound;
}
