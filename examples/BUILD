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
    name = "simple_tcp_echo_client",
    srcs = ["simple_tcp_echo_client.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//reactor:reactor_based_client",
        "//reactor:simple_tcp_service",
    ],
)

cc_binary(
    name = "simple_tcp_echo_server",
    srcs = ["simple_tcp_echo_server.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//:jmg_server_main",
        "//reactor",
        "//reactor:reactor_based_server",
        "//reactor:simple_tcp_service",
    ],
)
