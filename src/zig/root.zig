//! By convention, root.zig is the root source file when making a library.
const std = @import("std");

// Re-export database module
pub const database = struct {
    pub const SqliteHandler = @import("database/sqlitehandler.zig").SqliteHandler;
    pub const HistoryChat = @import("database/sqlitehandler.zig").HistoryChat;
};

// Re-export server module
pub const server = struct {
    pub const Server = @import("server/server.zig").Server;
    pub const startServer = @import("server/server.zig").startServer;
};

// Re-export MCP handler module
pub const mcp = struct {
    pub const MCPHandler = @import("mcphandler.zig").MCPHandler;
    pub const loadNvidiaToken = @import("mcphandler.zig").loadNvidiaToken;
};

pub fn add(a: i32, b: i32) i32 {
    return a + b;
}

test "basic add functionality" {
    try std.testing.expect(add(3, 7) == 10);
}
