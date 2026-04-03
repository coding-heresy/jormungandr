load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "test_gen",
    srcs = ["test_gen.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//:jmg",
        "@jinja2cpp",
    ],
)
