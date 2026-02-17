####################
# control blocks

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "control_blocks",
    hdrs = ["control_blocks.h"],
    include_prefix = "jmg/reactor",
    deps = ["//:jmg"],
)

cc_test(
    name = "test_control_blocks",
    size = "small",
    srcs = ["test_control_blocks.cpp"],
    deps = [
        ":control_blocks",
        "@com_google_googletest//:gtest_main",
    ],
)

####################
# io_uring

cc_library(
    name = "uring",
    srcs = ["uring.cpp"],
    hdrs = ["uring.h"],
    defines = ["_GNU_SOURCE"],
    include_prefix = "jmg/reactor",
    deps = [
        "//:jmg",
        "@liburing",
    ],
)

cc_test(
    name = "test_uring",
    size = "small",
    srcs = ["test_uring.cpp"],
    deps = [
        ":uring",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_ucontext",
    size = "small",
    srcs = ["test_ucontext.cpp"],
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

####################
# reactor

cc_library(
    name = "reactor",
    srcs = [
        "fiber.cpp",
        "reactor.cpp",
    ],
    hdrs = [
        "fiber.h",
        "reactor.h",
    ],
    include_prefix = "jmg/reactor",
    visibility = ["//visibility:public"],
    deps = [
        ":control_blocks",
        ":uring",
        # TODO(bd) figure out how to make boost.asio work with shadow
        # stacks or try a different thread pool implementation
        # "@boost.asio",
        "@bs_thread-pool//:thread_pool",
        "@dp_thread-pool//:thread_pool",
    ],
)

cc_test(
    name = "test_reactor",
    size = "small",
    srcs = ["test_reactor.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":reactor",
        "@com_google_googletest//:gtest_main",
    ],
)

####################
# support libraries

cc_library(
    name = "reactor_based_client",
    srcs = ["reactor_based_client.cpp"],
    hdrs = ["reactor_based_client.h"],
    include_prefix = "jmg/reactor",
    visibility = ["//visibility:public"],
    deps = [":reactor"],
)

cc_library(
    name = "reactor_based_server",
    srcs = ["reactor_based_server.cpp"],
    hdrs = ["reactor_based_server.h"],
    include_prefix = "jmg/reactor",
    visibility = ["//visibility:public"],
    deps = [":reactor"],
)

cc_library(
    name = "simple_tcp_service",
    srcs = ["simple_tcp_service.cpp"],
    hdrs = ["simple_tcp_service.h"],
    include_prefix = "jmg/reactor",
    deps = [":reactor"],
)

####################
# misc demo programs

cc_binary(
    name = "lookup_address",
    srcs = ["misc/lookup_address.cpp"],
    visibility = ["//visibility:public"],
    deps = [":reactor"],
)

cc_binary(
    name = "echo_client",
    srcs = ["misc/echo_client.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":reactor_based_client",
        ":simple_tcp_service",
    ],
)

cc_binary(
    name = "echo_server",
    srcs = ["misc/echo_server.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":reactor",
        ":reactor_based_server",
        ":simple_tcp_service",
        "//:jmg_server_main",
    ],
)
