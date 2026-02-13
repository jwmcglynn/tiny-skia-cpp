load("@rules_cc//cc:defs.bzl", "cc_library")

def _tiny_skia_cc_library_impl(name, srcs, hdrs, deps = [], copts = [], defines = [], visibility = ["//visibility:private"], **kwargs):
    cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        deps = deps,
        copts = copts,
        defines = defines,
        visibility = visibility,
        **kwargs,
    )

def tiny_skia_cc_library(**kwargs):
    _tiny_skia_cc_library_impl(**kwargs)
