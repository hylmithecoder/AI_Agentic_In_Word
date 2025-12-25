const std = @import("std");
const AgenticAIOnWord = @import("AgenticAIOnWord");
const print = std.debug.print;

const SERVER_PORT: u16 = 9910;

pub fn main() !void {
    // Initialize allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    print("[Server] Starting Agentic AI WebSocket server on port {d}\n", .{SERVER_PORT});

    // Load NVIDIA API token from .env file
    var token_owned: ?[]const u8 = null;
    const nvidia_token: []const u8 = AgenticAIOnWord.mcp.loadNvidiaToken(allocator, ".env") catch |err| blk: {
        print("[Server] Warning: Could not load NVIDIA API token: {}\n", .{err});
        print("[Server] AI features will be disabled. Make sure .env file exists with NVIDIA_API_KEY=your_key\n", .{});
        break :blk "";
    };
    if (nvidia_token.len > 0) {
        token_owned = nvidia_token;
    }
    defer if (token_owned) |t| allocator.free(t);

    print("[Server] NVIDIA API token loaded ({d} chars)\n", .{nvidia_token.len});

    // Initialize MCP handler
    var mcp_handler = AgenticAIOnWord.mcp.MCPHandler.init(allocator, nvidia_token);
    defer mcp_handler.deinit();

    // Initialize database
    var db = try AgenticAIOnWord.database.SqliteHandler.init(allocator, "ms_word.db");
    defer db.deinit();

    // Start WebSocket server
    try AgenticAIOnWord.server.startServer(allocator, &db, &mcp_handler, SERVER_PORT);
}

test "simple test" {
    const gpa = std.testing.allocator;
    var list: std.ArrayList(i32) = .empty;
    defer list.deinit(gpa);
    try list.append(gpa, 42);
    try std.testing.expectEqual(@as(i32, 42), list.pop());
}

test "fuzz example" {
    const Context = struct {
        fn testOne(context: @This(), input: []const u8) anyerror!void {
            _ = context;
            try std.testing.expect(!std.mem.eql(u8, "canyoufindme", input));
        }
    };
    try std.testing.fuzz(Context{}, Context.testOne, .{});
}
