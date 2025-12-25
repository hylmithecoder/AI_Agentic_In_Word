const std = @import("std");
const sqlite = @import("sqlite");
const uuid = @import("uuid");

/// History chat record structure
pub const HistoryChat = struct {
    id: i64,
    uuid_str: []const u8,
    message: []const u8,
    timestamp: []const u8,
    file: []const u8,
};

/// Optimized SQLite handler using low-level C API
pub const SqliteHandler = struct {
    const Self = @This();
    const c = sqlite.c;

    db: ?*c.sqlite3,
    allocator: std.mem.Allocator,

    /// Initialize database connection
    pub fn init(allocator: std.mem.Allocator, db_path: [:0]const u8) !Self {
        var db: ?*c.sqlite3 = null;
        const result = c.sqlite3_open(db_path.ptr, &db);

        if (result != c.SQLITE_OK) {
            if (db != null) {
                _ = c.sqlite3_close(db);
            }
            return error.SqliteOpenFailed;
        }

        var handler = Self{
            .db = db,
            .allocator = allocator,
        };

        // Create tables on init
        try handler.createTables();

        return handler;
    }

    /// Close database connection
    pub fn deinit(self: *Self) void {
        if (self.db) |db| {
            _ = c.sqlite3_close(db);
        }
        self.db = null;
    }

    /// Create required tables
    fn createTables(self: *Self) !void {
        const create_sql = "CREATE TABLE IF NOT EXISTS history_chat (" ++
            "id INTEGER PRIMARY KEY AUTOINCREMENT," ++
            "uuid TEXT NOT NULL UNIQUE," ++
            "message TEXT NOT NULL," ++
            "timestamp TEXT NOT NULL," ++
            "file TEXT DEFAULT '')";

        var err_msg: [*c]u8 = null;
        const result = c.sqlite3_exec(self.db, create_sql, null, null, &err_msg);

        if (result != c.SQLITE_OK) {
            if (err_msg != null) {
                c.sqlite3_free(err_msg);
            }
            return error.SqliteExecFailed;
        }

        std.debug.print("[SQLite] Tables created/verified\n", .{});
    }

    /// Generate UUID v4
    fn generateUuid() [36]u8 {
        const uuid4 = uuid.v4.new();
        return uuid.urn.serialize(uuid4);
    }

    fn getCurrentTimestamp() i64 {
        const currentTime = std.time.timestamp();
        return currentTime;
    }

    /// Get current timestamp as human-readable string (YYYY-MM-DD HH:MM:SS)
    fn getCurrentTimestampString() [19]u8 {
        const ts = std.time.timestamp();
        const epoch_seconds = std.time.epoch.EpochSeconds{ .secs = @intCast(ts) };
        const day_seconds = epoch_seconds.getDaySeconds();
        const year_day = epoch_seconds.getEpochDay().calculateYearDay();
        const month_day = year_day.calculateMonthDay();

        var buf: [19]u8 = undefined;
        _ = std.fmt.bufPrint(&buf, "{d:0>4}-{d:0>2}-{d:0>2} {d:0>2}:{d:0>2}:{d:0>2}", .{
            year_day.year,
            month_day.month.numeric(),
            month_day.day_index + 1,
            day_seconds.getHoursIntoDay(),
            day_seconds.getMinutesIntoHour(),
            day_seconds.getSecondsIntoMinute(),
        }) catch unreachable;
        return buf;
    }

    /// Insert a new chat message
    pub fn insertHistoryChat(self: *Self, message: []const u8, file: []const u8) !void {
        const uuid_str = generateUuid();
        const timestampstring: [19]u8 = getCurrentTimestampString();
        // const timestamp = getCurrentTimestamp();

        std.debug.print("Time Stamp string {s}", .{timestampstring});
        // std.debug.print("Time Stamp {d}", .{timestamp});
        const insert_sql = "INSERT INTO history_chat (uuid, message, timestamp, file) VALUES (?, ?, ?, ?)";

        var stmt: ?*c.sqlite3_stmt = null;
        var rc = c.sqlite3_prepare_v2(self.db, insert_sql, -1, &stmt, null);

        if (rc != c.SQLITE_OK) {
            return error.SqlitePrepareFailed;
        }
        defer _ = c.sqlite3_finalize(stmt);

        // Bind parameters
        _ = c.sqlite3_bind_text(stmt, 1, &uuid_str, 36, null);
        _ = c.sqlite3_bind_text(stmt, 2, message.ptr, @intCast(message.len), null);
        _ = c.sqlite3_bind_text(stmt, 3, &timestampstring, 19, null);
        _ = c.sqlite3_bind_text(stmt, 4, file.ptr, @intCast(file.len), null);

        rc = c.sqlite3_step(stmt);
        if (rc != c.SQLITE_DONE) {
            return error.SqliteStepFailed;
        }

        std.debug.print("[SQLite] Inserted message with UUID: {s}\n", .{uuid_str});
    }

    /// Delete all history
    pub fn deleteAll(self: *Self) !void {
        var err_msg: [*c]u8 = null;
        const result = c.sqlite3_exec(self.db, "DELETE FROM history_chat", null, null, &err_msg);

        if (result != c.SQLITE_OK) {
            if (err_msg != null) {
                c.sqlite3_free(err_msg);
            }
            return error.SqliteExecFailed;
        }
        std.debug.print("[SQLite] All history deleted\n", .{});
    }

    /// Get all history chat records
    pub fn getTables(self: *Self) !std.ArrayList(HistoryChat) {
        const select_sql = "SELECT * FROM history_chat";

        var stmt: ?*c.sqlite3_stmt = null;
        const rc = c.sqlite3_prepare_v2(self.db, select_sql, -1, &stmt, null);

        if (rc != c.SQLITE_OK) {
            return error.SqlitePrepareFailed;
        }
        defer _ = c.sqlite3_finalize(stmt);

        var results: std.ArrayList(HistoryChat) = .empty;

        while (c.sqlite3_step(stmt) == c.SQLITE_ROW) {
            const id = c.sqlite3_column_int64(stmt, 0);
            // const timestamp = c.sqlite3_column_int64(stmt, 3);

            // Get text columns as slices
            const uuid_ptr = c.sqlite3_column_text(stmt, 1);
            const uuid_len: usize = @intCast(c.sqlite3_column_bytes(stmt, 1));

            const msg_ptr = c.sqlite3_column_text(stmt, 2);
            const msg_len: usize = @intCast(c.sqlite3_column_bytes(stmt, 2));

            const ts_ptr = c.sqlite3_column_text(stmt, 3);
            const ts_len: usize = @intCast(c.sqlite3_column_bytes(stmt, 3));

            const file_ptr = c.sqlite3_column_text(stmt, 4);
            const file_len: usize = @intCast(c.sqlite3_column_bytes(stmt, 4));

            // Copy strings to owned memory
            const uuid_copy = try self.allocator.alloc(u8, uuid_len);
            if (uuid_ptr != null) {
                @memcpy(uuid_copy, uuid_ptr[0..uuid_len]);
            }

            const msg_copy = try self.allocator.alloc(u8, msg_len);
            if (msg_ptr != null) {
                @memcpy(msg_copy, msg_ptr[0..msg_len]);
            }

            const ts_copy = try self.allocator.alloc(u8, ts_len);
            if (ts_ptr != null) {
                @memcpy(ts_copy, ts_ptr[0..ts_len]);
            }

            const file_copy = try self.allocator.alloc(u8, file_len);
            if (file_ptr != null) {
                @memcpy(file_copy, file_ptr[0..file_len]);
            }

            try results.append(self.allocator, .{
                .id = id,
                .uuid_str = uuid_copy,
                .message = msg_copy,
                .timestamp = ts_copy,
                .file = file_copy,
            });
        }

        return results;
    }

    /// Free history chat records
    pub fn freeHistory(self: *Self, records: *std.ArrayList(HistoryChat)) void {
        for (records.items) |item| {
            self.allocator.free(item.uuid_str);
            self.allocator.free(item.message);
            self.allocator.free(item.timestamp);
            self.allocator.free(item.file);
        }
        records.deinit(self.allocator);
    }
};
