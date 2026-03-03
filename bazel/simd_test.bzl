"""Helpers for generating paired native/scalar cc_test targets."""

load("@rules_cc//cc:defs.bzl", "cc_test")


def _rewrite_dep(dep, mode):
    if dep == "//src:tiny_skia_lib" or dep == "@//src:tiny_skia_lib":
        return "//src:tiny_skia_lib_%s" % mode
    return dep


def _rewrite_deps(deps, mode):
    return [_rewrite_dep(dep, mode) for dep in deps]


def tiny_skia_dual_mode_cc_test(
        name,
        srcs,
        deps,
        copts = [],
        data = [],
        tags = [],
        **kwargs):
    native_name = name + "_native"
    scalar_name = name + "_scalar"

    cc_test(
        name = native_name,
        srcs = srcs,
        deps = _rewrite_deps(deps, "native"),
        copts = copts,
        data = data,
        tags = tags + ["simd_mode=native"],
        **kwargs
    )

    cc_test(
        name = scalar_name,
        srcs = srcs,
        deps = _rewrite_deps(deps, "scalar"),
        copts = copts,
        data = data,
        tags = tags + ["simd_mode=scalar"],
        **kwargs
    )

    native.test_suite(
        name = name,
        tests = [
            ":" + native_name,
            ":" + scalar_name,
        ],
        tags = tags,
    )
