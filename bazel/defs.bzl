load("@rules_cc//cc:defs.bzl", "cc_library")

_SIMD_NATIVE_X86_COPTS = select({
    "//bazel/config:simd_native_x86_64": [
        "-mavx2",
        "-mfma",
    ],
    "//bazel/config:simd_native_x86_32": [
        "-mavx2",
        "-mfma",
    ],
    "//conditions:default": [],
})

_OPT_MODE_COPTS = select({
    "//bazel/config:compilation_mode_opt": ["-O3"],
    "//conditions:default": [],
})


def _tiny_skia_cc_library_impl(
        name,
        srcs,
        hdrs,
        deps = [],
        copts = [],
        defines = [],
        visibility = ["//visibility:private"],
        **kwargs):
    cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        deps = deps,
        copts = ["-std=c++20", "-ffp-contract=off"] + _OPT_MODE_COPTS + _SIMD_NATIVE_X86_COPTS + copts,
        defines = defines,
        visibility = visibility,
        **kwargs,
    )

def tiny_skia_cc_library(**kwargs):
    _tiny_skia_cc_library_impl(**kwargs)
