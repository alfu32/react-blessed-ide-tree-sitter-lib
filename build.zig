const std = @import("std");

fn addGrammarSources(b: *std.Build, step: *std.Build.Step.Compile, dir_path: []const u8) !void {
    const gpa = std.heap.page_allocator;
    var dir = try std.fs.cwd().openIterableDir(dir_path, .{});
    defer dir.close();

    var it = dir.iterate();
    while (try it.next()) |entry| {
        if (entry.kind == .file) {
            if (std.mem.endsWith(u8, entry.name, "parser.c") or
                std.mem.endsWith(u8, entry.name, "scanner.c"))
                {
                    const full_path = try std.fs.path.join(gpa, &.{dir_path, entry.name});
                    step.addCSourceFile(.{ .file = .{ .path = full_path }, .flags = &[_][]const u8{} });
                }
        } else if (entry.kind == .directory) {
            const subpath = try std.fs.path.join(gpa, &.{dir_path, entry.name});
            try addGrammarSources(b, step, subpath);
        }
    }
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addSharedLibrary(.{
        .name = "tree_sitter_tokenizer",
        .root_source_file = .{ .path = "src/api.c" },
        .target = target,
        .optimize = optimize,
    });

    // Include project headers
    lib.addIncludePath(.{ .path = "src" });

    // Include vcpkg headers (platform-specific path autodetection)
    const platform = b.host.target.getOsTag();
    const vcpkg_base = "vcpkg_installed";
    const triplet = switch (platform) {
        .linux => "x64-linux",
        .windows => "x64-windows",
        .macos => "x64-osx",
        else => "x64-linux",
    };

    const vcpkg_include = std.fmt.allocPrint(b.allocator, "{s}/{s}/include", .{ vcpkg_base, triplet }) catch unreachable;
    const vcpkg_lib = std.fmt.allocPrint(b.allocator, "{s}/{s}/lib", .{ vcpkg_base, triplet }) catch unreachable;

    lib.addIncludePath(.{ .path = vcpkg_include });
    lib.addLibraryPath(.{ .path = vcpkg_lib });
    lib.linkSystemLibrary("tree-sitter");

    // Add grammar sources dynamically
    const grammars_path = "grammars";
    if (std.fs.cwd().openDir(grammars_path, .{}) catch null) |dir| {
        dir.close();
        addGrammarSources(b, lib, grammars_path) catch {};
    } else {
        std.debug.print("No grammars directory found.\n", .{});
    }

    b.installArtifact(lib);
}
