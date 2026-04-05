load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:jmgc.bzl", "cc_jmg_library", "proto_jmg_library")

####################
# control blocks

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

# reactor-based client and server

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

# simple TCP service

cc_library(
    name = "simple_tcp_service",
    srcs = ["simple_tcp_service.cpp"],
    hdrs = ["simple_tcp_service.h"],
    include_prefix = "jmg/reactor",
    visibility = ["//visibility:public"],
    deps = [":reactor"],
)

# simple RPC service

filegroup(
    name = "rpc_common_objects_idl",
    srcs = ["rpc_common.yaml"],
    visibility = ["//visibility:private"],
)

cc_jmg_library(
    name = "rpc_common_objects_cbe_cc",
    src = ":rpc_common_objects_idl",
    encoding = "cbe",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "simple_rpc_service",
    hdrs = [
        "simple_rpc_client.h",
        "simple_rpc_service.h",
    ],
    include_prefix = "jmg/reactor",
    visibility = ["//visibility:public"],
    deps = [
        ":rpc_common_objects_cbe_cc",
        ":simple_tcp_service",
    ],
)
