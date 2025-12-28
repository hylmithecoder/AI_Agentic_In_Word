const std = @import("std");
const net = std.net;
const database = @import("../database/sqlitehandler.zig");
const mcp = @import("../mcphandler.zig");
const Sha1 = std.crypto.hash.Sha1;
const base64 = std.base64;

const MAX_HEADER_SIZE: usize = 8192;
const MAX_FRAME_SIZE: usize = 65536;
const WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/// WebSocket opcodes
const Opcode = enum(u4) {
    continuation = 0x0,
    text = 0x1,
    binary = 0x2,
    close = 0x8,
    ping = 0x9,
    pong = 0xA,
};

/// WebSocket frame structure
const WebSocketFrame = struct {
    fin: bool,
    opcode: Opcode,
    mask: bool,
    payload: []const u8,
};

/// WebSocket Server for Agentic AI on Word
pub const Server = struct {
    const Self = @This();

    allocator: std.mem.Allocator,
    db: *database.SqliteHandler,
    mcp_handler: *mcp.MCPHandler,
    server: ?net.Server,
    active_connection: ?net.Stream,

    pub fn init(allocator: std.mem.Allocator, db: *database.SqliteHandler, mcp_handler: *mcp.MCPHandler) Self {
        return Self{
            .allocator = allocator,
            .db = db,
            .mcp_handler = mcp_handler,
            .server = null,
            .active_connection = null,
        };
    }

    pub fn deinit(self: *Self) void {
        if (self.active_connection) |conn| {
            conn.close();
        }
        if (self.server) |*s| {
            s.deinit();
        }
    }

    /// Start the WebSocket server
    pub fn run(self: *Self, port: u16) !void {
        const address = net.Address.initIp4(.{ 127, 0, 0, 1 }, port);
        self.server = try address.listen(.{
            .reuse_address = true,
        });

        std.debug.print("[WebSocket] Listening on ws://localhost:{d}\n", .{port});

        while (true) {
            const conn = self.server.?.accept() catch |err| {
                std.debug.print("[WebSocket] Accept error: {}\n", .{err});
                continue;
            };

            self.handleConnection(conn) catch |err| {
                std.debug.print("[WebSocket] Handler error: {}\n", .{err});
            };
        }
    }

    /// Handle incoming connection
    fn handleConnection(self: *Self, conn: net.Server.Connection) !void {
        var buffer: [MAX_HEADER_SIZE]u8 = undefined;
        const len = std.posix.recv(conn.stream.handle, &buffer, 0) catch |err| {
            std.debug.print("[WebSocket] recv error: {}\n", .{err});
            conn.stream.close();
            return err;
        };

        if (len == 0) {
            conn.stream.close();
            return;
        }

        const request = buffer[0..len];

        // Check if this is a WebSocket upgrade request
        if (std.mem.indexOf(u8, request, "Upgrade: websocket") != null or
            std.mem.indexOf(u8, request, "Upgrade: Websocket") != null)
        {
            try self.handleWebSocketUpgrade(conn.stream, request);
        } else {
            // Handle as regular HTTP request
            try self.handleHttpRequest(conn.stream, request);
            conn.stream.close();
        }
    }

    /// Handle WebSocket upgrade handshake
    fn handleWebSocketUpgrade(self: *Self, stream: net.Stream, request: []const u8) !void {
        // Extract Sec-WebSocket-Key
        const key_header = "Sec-WebSocket-Key: ";
        const key_start = std.mem.indexOf(u8, request, key_header) orelse {
            std.debug.print("[WebSocket] No Sec-WebSocket-Key found\n", .{});
            stream.close();
            return;
        };

        const key_value_start = key_start + key_header.len;
        const key_end = std.mem.indexOfPos(u8, request, key_value_start, "\r\n") orelse {
            stream.close();
            return;
        };

        const websocket_key = request[key_value_start..key_end];

        // Generate accept key: base64(sha1(key + GUID))
        var sha1 = Sha1.init(.{});
        sha1.update(websocket_key);
        sha1.update(WEBSOCKET_GUID);
        const hash = sha1.finalResult();

        var accept_key: [28]u8 = undefined;
        _ = base64.standard.Encoder.encode(&accept_key, &hash);

        // Send upgrade response
        const response = "HTTP/1.1 101 Switching Protocols\r\n" ++
            "Upgrade: websocket\r\n" ++
            "Connection: Upgrade\r\n" ++
            "Sec-WebSocket-Accept: ";

        _ = try stream.writeAll(response);
        _ = try stream.writeAll(&accept_key);
        _ = try stream.writeAll("\r\n\r\n");

        std.debug.print("[WebSocket] Connection upgraded successfully\n", .{});

        // Store active connection
        self.active_connection = stream;

        // Handle WebSocket frames
        self.handleWebSocketFrames(stream) catch |err| {
            std.debug.print("[WebSocket] Frame handling error: {}\n", .{err});
        };

        self.active_connection = null;
        stream.close();
    }

    /// Handle WebSocket frames in a loop
    fn handleWebSocketFrames(self: *Self, stream: net.Stream) !void {
        var frame_buffer: [MAX_FRAME_SIZE]u8 = undefined;

        while (true) {
            // Read frame header
            const header_len = std.posix.recv(stream.handle, frame_buffer[0..2], 0) catch |err| {
                std.debug.print("[WebSocket] Header recv error: {}\n", .{err});
                return err;
            };

            if (header_len < 2) {
                std.debug.print("[WebSocket] Connection closed by client\n", .{});
                return;
            }

            const byte0 = frame_buffer[0];
            const byte1 = frame_buffer[1];

            const fin = (byte0 & 0x80) != 0;
            const opcode_val = byte0 & 0x0F;
            const opcode: Opcode = @enumFromInt(opcode_val);
            const masked = (byte1 & 0x80) != 0;
            var payload_len: usize = byte1 & 0x7F;

            // Extended payload length
            if (payload_len == 126) {
                _ = try std.posix.recv(stream.handle, frame_buffer[2..4], 0);
                payload_len = (@as(usize, frame_buffer[2]) << 8) | @as(usize, frame_buffer[3]);
            } else if (payload_len == 127) {
                _ = try std.posix.recv(stream.handle, frame_buffer[2..10], 0);
                payload_len = 0;
                for (0..8) |i| {
                    payload_len = (payload_len << 8) | @as(usize, frame_buffer[2 + i]);
                }
            }

            // Read mask key if present
            var mask_key: [4]u8 = undefined;
            if (masked) {
                _ = try std.posix.recv(stream.handle, &mask_key, 0);
            }

            // Read payload
            if (payload_len > MAX_FRAME_SIZE) {
                std.debug.print("[WebSocket] Payload too large: {}\n", .{payload_len});
                return error.PayloadTooLarge;
            }

            var payload = try self.allocator.alloc(u8, payload_len);
            defer self.allocator.free(payload);

            var total_read: usize = 0;
            while (total_read < payload_len) {
                const read = std.posix.recv(stream.handle, payload[total_read..], 0) catch |err| {
                    std.debug.print("[WebSocket] Payload recv error: {}\n", .{err});
                    return err;
                };
                if (read == 0) return error.ConnectionClosed;
                total_read += read;
            }

            // Unmask payload
            if (masked) {
                for (payload, 0..) |*byte, i| {
                    byte.* ^= mask_key[i % 4];
                }
            }

            // Handle frame by opcode
            switch (opcode) {
                .text => {
                    std.debug.print("[WebSocket] Received text: {s}\n", .{payload});
                    try self.handleTextMessage(stream, payload);
                },
                .binary => {
                    std.debug.print("[WebSocket] Received binary frame\n", .{});
                },
                .close => {
                    std.debug.print("[WebSocket] Close frame received\n", .{});
                    // Send close frame back
                    try self.sendFrame(stream, .close, "");
                    return;
                },
                .ping => {
                    std.debug.print("[WebSocket] Ping received, sending pong\n", .{});
                    try self.sendFrame(stream, .pong, payload);
                },
                .pong => {
                    std.debug.print("[WebSocket] Pong received\n", .{});
                },
                else => {
                    std.debug.print("[WebSocket] Unknown opcode: {}\n", .{opcode_val});
                },
            }

            _ = fin;
        }
    }

    /// Handle text message from WebSocket client
    fn handleTextMessage(self: *Self, stream: net.Stream, message: []const u8) !void {
        // Parse JSON message
        const parsed = std.json.parseFromSlice(std.json.Value, self.allocator, message, .{}) catch {
            try self.sendJsonResponse(stream, .{
                .id = "unknown",
                .status = "error",
                .content = "Invalid JSON format",
            });
            return;
        };
        defer parsed.deinit();

        const root = parsed.value.object;

        // Extract request fields
        const id = if (root.get("id")) |v| v.string else "unknown";
        const msg_type = if (root.get("type")) |v| v.string else "unknown";

        std.debug.print("[WebSocket] Processing request: id={s}, type={s}\n", .{ id, msg_type });

        // Route to appropriate handler
        if (std.mem.eql(u8, msg_type, "analyze") or
            std.mem.eql(u8, msg_type, "explain") or
            std.mem.eql(u8, msg_type, "review") or
            std.mem.eql(u8, msg_type, "refactor"))
        {
            const file_path: []const u8 = if (root.get("file_path")) |v| v.string else "";
            const currentFile: []const u8 = if (root.get("current_file")) |v| v.string else "";
            const content: []const u8 = if (root.get("content")) |v| v.string else "";
            const prompt: []const u8 = if (root.get("prompt")) |v| v.string else "";

            try self.db.insertHistoryChat(prompt, file_path, "user", currentFile);
            try self.mcp_handler.processRequest(
                self.allocator,
                stream,
                id,
                msg_type,
                file_path,
                content,
                prompt,
            );
        } else if (std.mem.eql(u8, msg_type, "health")) {
            try self.sendJsonResponse(stream, .{
                .id = id,
                .status = "ok",
                .content = "WebSocket server is running",
            });
        } else if (std.mem.eql(u8, msg_type, "history")) {
            try self.handleGetHistory(stream);
        } else {
            try self.sendJsonResponse(stream, .{
                .id = id,
                .status = "error",
                .content = "Unknown message type",
            });
        }
    }

    /// Escape a string for JSON output
    fn escapeJsonString(self: *Self, builder: *std.ArrayList(u8), str: []const u8) !void {
        for (str) |ch| {
            switch (ch) {
                '"' => try builder.appendSlice(self.allocator, "\\\""),
                '\\' => try builder.appendSlice(self.allocator, "\\\\"),
                '\n' => try builder.appendSlice(self.allocator, "\\n"),
                '\r' => try builder.appendSlice(self.allocator, "\\r"),
                '\t' => try builder.appendSlice(self.allocator, "\\t"),
                0x08 => try builder.appendSlice(self.allocator, "\\b"), // backspace
                0x0C => try builder.appendSlice(self.allocator, "\\f"), // form feed
                else => {
                    if (ch < 0x20) {
                        // Encode other control chars as \u00XX
                        var buf: [6]u8 = undefined;
                        _ = std.fmt.bufPrint(&buf, "\\u00{x:0>2}", .{ch}) catch continue;
                        try builder.appendSlice(self.allocator, &buf);
                    } else {
                        try builder.append(self.allocator, ch);
                    }
                },
            }
        }
    }

    /// Handle history request
    fn handleGetHistory(self: *Self, stream: net.Stream) !void {
        var records = self.db.getTables() catch {
            try self.sendJsonResponse(stream, .{
                .status = "error",
                .content = "Database error",
            });
            return;
        };
        defer self.db.freeHistory(&records);

        // Use dynamic ArrayList for building JSON
        var json_builder = try std.ArrayList(u8).initCapacity(self.allocator, 8192);
        defer json_builder.deinit(self.allocator);

        // Start JSON object
        try json_builder.appendSlice(self.allocator, "{\"type\":\"history\",\"status\":\"ok\",\"data\":[");

        // Limit records to prevent excessive response size
        const max_records: usize = 50;
        const record_count = @min(records.items.len, max_records);

        for (records.items[0..record_count], 0..) |item, i| {
            if (i > 0) try json_builder.appendSlice(self.allocator, ",");

            // Build each record object
            try json_builder.appendSlice(self.allocator, "{\"uuid\":\"");
            try self.escapeJsonString(&json_builder, item.uuid_str);

            try json_builder.appendSlice(self.allocator, "\",\"message\":\"");
            try self.escapeJsonString(&json_builder, item.message);

            try json_builder.appendSlice(self.allocator, "\",\"timestamp\":\"");
            try self.escapeJsonString(&json_builder, item.timestamp);

            try json_builder.appendSlice(self.allocator, "\",\"role\":\"");
            try self.escapeJsonString(&json_builder, item.role);

            try json_builder.appendSlice(self.allocator, "\",\"current_file\":\"");
            try self.escapeJsonString(&json_builder, item.current_File);

            try json_builder.appendSlice(self.allocator, "\"}");
        }

        // Close JSON array and object
        try json_builder.appendSlice(self.allocator, "]}");

        std.debug.print("[WebSocket] History response: {d} records, {d} bytes\n", .{ record_count, json_builder.items.len });
        try self.sendFrame(stream, .text, json_builder.items);
    }

    /// Send JSON response helper
    fn sendJsonResponse(self: *Self, stream: net.Stream, response: anytype) !void {
        var json_buf: [4096]u8 = undefined;
        var fbs = std.io.fixedBufferStream(&json_buf);
        const writer = fbs.writer();

        // Manually build JSON for simple response struct
        try writer.writeAll("{");
        inline for (std.meta.fields(@TypeOf(response)), 0..) |field, i| {
            if (i > 0) try writer.writeAll(",");
            try writer.writeAll("\"");
            try writer.writeAll(field.name);
            try writer.writeAll("\":\"");
            const value = @field(response, field.name);
            // Escape the value
            for (value) |ch| {
                switch (ch) {
                    '"' => try writer.writeAll("\\\""),
                    '\\' => try writer.writeAll("\\\\"),
                    '\n' => try writer.writeAll("\\n"),
                    '\r' => try writer.writeAll("\\r"),
                    '\t' => try writer.writeAll("\\t"),
                    else => try writer.writeByte(ch),
                }
            }
            try writer.writeAll("\"");
        }
        try writer.writeAll("}");

        try self.sendFrame(stream, .text, fbs.getWritten());
    }

    /// Send WebSocket frame
    pub fn sendFrame(self: *Self, stream: net.Stream, opcode: Opcode, payload: []const u8) !void {
        _ = self;
        var header: [10]u8 = undefined;
        var header_len: usize = 2;

        // FIN bit + opcode
        header[0] = 0x80 | @as(u8, @intFromEnum(opcode));

        // Payload length (server frames are not masked)
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

        // Send header
        _ = try stream.writeAll(header[0..header_len]);

        // Send payload
        if (payload.len > 0) {
            _ = try stream.writeAll(payload);
        }
    }

    /// Handle regular HTTP request (for backwards compatibility)
    fn handleHttpRequest(self: *Self, stream: net.Stream, request: []const u8) !void {
        var lines = std.mem.splitSequence(u8, request, "\r\n");
        const first_line = lines.first();

        var parts = std.mem.splitScalar(u8, first_line, ' ');
        const method = parts.next() orelse return;
        const path = parts.next() orelse return;

        std.debug.print("[HTTP] {s} {s}\n", .{ method, path });

        if (std.mem.eql(u8, method, "GET")) {
            if (std.mem.eql(u8, path, "/health")) {
                try self.sendHttpJson(stream, 200, "{\"status\":\"ok\",\"service\":\"agentic-ai\",\"websocket\":true}");
            } else {
                try self.sendHttpJson(stream, 404, "{\"error\":\"Not Found\"}");
            }
        } else {
            try self.sendHttpJson(stream, 405, "{\"error\":\"Method Not Allowed\"}");
        }
    }

    /// Send HTTP JSON response
    fn sendHttpJson(self: *Self, stream: net.Stream, status: u16, body: []const u8) !void {
        _ = self;
        const status_text = switch (status) {
            200 => "OK",
            404 => "Not Found",
            405 => "Method Not Allowed",
            else => "Unknown",
        };

        var response_buf: [8192]u8 = undefined;
        const response = std.fmt.bufPrint(&response_buf, "HTTP/1.1 {d} {s}\r\n" ++
            "Content-Type: application/json\r\n" ++
            "Content-Length: {d}\r\n" ++
            "Access-Control-Allow-Origin: *\r\n" ++
            "Connection: close\r\n" ++
            "\r\n" ++
            "{s}", .{ status, status_text, body.len, body }) catch return;

        _ = try stream.writeAll(response);
    }
};

/// Start the WebSocket server with given database and MCP handler
pub fn startServer(
    allocator: std.mem.Allocator,
    db: *database.SqliteHandler,
    mcp_handler: *mcp.MCPHandler,
    port: u16,
) !void {
    var server = Server.init(allocator, db, mcp_handler);
    defer server.deinit();
    try server.run(port);
}
