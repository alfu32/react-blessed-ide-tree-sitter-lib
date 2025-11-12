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
                    std.debug.print("[+] Adding grammar source: {s}\n", .{full_path});
                    step.addCSourceFile(.{ .file = .{ .path = full_path }, .flags = &[_][]const u8{} });
                }
        } else if (entry.kind == .directory) {
            const subpath = try std.fs.path.join(gpa, &.{dir_path, entry.name});
            try addGrammarSources(b, step, subpath);
        }
    }
}

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    const triplet_opt = b.option([]const u8, "triplet", "Target triplet (linux64-gnu, windows64-msvc, macos64, etc.)");
    const triplet = triplet_opt orelse "linux64-gnu";

    const target = if (std.mem.eql(u8, triplet, "linux64-gnu"))
        b.resolveTargetQuery(.{ .os_tag = .linux, .abi = .gnu, .cpu_arch = .x86_64 })
    else if (std.mem.eql(u8, triplet, "windows64-msvc"))
            b.resolveTargetQuery(.{ .os_tag = .windows, .abi = .msvc, .cpu_arch = .x86_64 })
        else if (std.mem.eql(u8, triplet, "macos64"))
                b.resolveTargetQuery(.{ .os_tag = .macos, .cpu_arch = .x86_64 })
            else blk: {
                    std.debug.print("[!] Unknown triplet '{s}', defaulting to linux64-gnu\n", .{triplet});
                    break :blk b.resolveTargetQuery(.{ .os_tag = .linux, .abi = .gnu, .cpu_arch = .x86_64 });
                };

    const lib = b.addSharedLibrary(.{
        .name = "tree_sitter_tokenizer",
        .root_source_file = .{ .path = "src/api.c" },
        .target = target,
        .optimize = optimize,
    });

    lib.addIncludePath(.{ .path = "src" });

    const vcpkg_base = "vcpkg_installed";
    const vcpkg_include = std.fmt.allocPrint(b.allocator, "{s}/{s}/include", .{ vcpkg_base, triplet }) catch unreachable;
    const vcpkg_lib = std.fmt.allocPrint(b.allocator, "{s}/{s}/lib", .{ vcpkg_base, triplet }) catch unreachable;

    lib.addIncludePath(.{ .path = vcpkg_include });
    lib.addLibraryPath(.{ .path = vcpkg_lib });
    lib.linkSystemLibrary("tree-sitter");

    const grammars_path = "grammars";
    if (std.fs.cwd().openDir(grammars_path, .{}) catch null) |dir| {
        dir.close();
        addGrammarSources(b, lib, grammars_path) catch {};
    } else {
        std.debug.print("[!] No grammars directory found.\n", .{});
    }

    lib.verbose_cc = true;
    b.installArtifact(lib);

    const step = b.step("build-lib", "Build tree-sitter-tokenizer shared library");
    step.dependOn(&lib.step);
}
