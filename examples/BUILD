load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "cmdline_demo",
    srcs = ["cmdline_demo.cpp"],
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "//:jmg",
    ],
)
