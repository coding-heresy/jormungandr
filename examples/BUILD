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

cc_binary(
    name = "lookup_address",
    srcs = ["lookup_address.cpp"],
    visibility = ["//visibility:public"],
    deps = ["//reactor"],
)

cc_binary(
    name = "echo_client",
    srcs = ["echo_client.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//reactor:reactor_based_client",
        "//reactor:simple_tcp_service",
    ],
)

cc_binary(
    name = "echo_server",
    srcs = ["echo_server.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//reactor:reactor",
        "//reactor:reactor_based_server",
        "//reactor:simple_tcp_service",
        "//:jmg_server_main",
    ],
)
