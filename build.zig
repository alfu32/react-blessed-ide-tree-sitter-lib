const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addSharedLibrary(.{
        .name = "tree_sitter_tokenizer",
        .root_source_file = .{ .path = "src/api.c" },
        .target = target,
        .optimize = optimize,
    });

    lib.addIncludePath(.{ .path = "src" });
    lib.linkSystemLibrary("tree-sitter");
    b.installArtifact(lib);
}
