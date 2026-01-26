
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
    srcs = ["test_control_blocks.cpp"],
    deps = [
        "@com_google_googletest//:gtest_main",
        ":control_blocks",
    ],
    size = "small",
)

####################
# io_uring

cc_library(
    name = "uring",
    hdrs = ["uring.h"],
    include_prefix = "jmg/reactor",
    srcs = ["uring.cpp"],
    defines = ["_GNU_SOURCE"],
    deps = [
        "//:jmg",
        "@liburing",
    ],
)

cc_test(
    name = "test_uring",
    srcs = ["test_uring.cpp"],
    deps = [
        "@com_google_googletest//:gtest_main",
        ":uring",
    ],
    size = "small",
)

cc_test(
    name = "test_ucontext",
    srcs = ["test_ucontext.cpp"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "//:jmg",
    ],
    size = "small",
)

####################
# reactor

cc_library(
    name = "reactor",
    visibility = ["//visibility:public"],
    srcs = [
        "fiber.cpp",
        "reactor.cpp",
    ],
    hdrs = [
        "fiber.h",
        "reactor.h"
    ],
    include_prefix = "jmg/reactor",
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
    visibility = ["//visibility:public"],
    srcs = ["test_reactor.cpp"],
    deps = [
        "@com_google_googletest//:gtest_main",
        ":reactor",
    ],
    size = "small",
)

####################
# support libraries

cc_library(
    name = "reactor_based_client",
    visibility = ["//visibility:public"],
    hdrs = ["reactor_based_client.h"],
    srcs = ["reactor_based_client.cpp"],
    include_prefix = "jmg/reactor",
    deps = [":reactor"],
)

cc_library(
    name = "reactor_based_server",
    visibility = ["//visibility:public"],
    hdrs = ["reactor_based_server.h"],
    srcs = ["reactor_based_server.cpp"],
    include_prefix = "jmg/reactor",
    deps = [":reactor"],
)

cc_library(
    name = "simple_tcp_service",
    hdrs = ["simple_tcp_service.h"],
    srcs = ["simple_tcp_service.cpp"],
    include_prefix = "jmg/reactor",
    deps = [":reactor"],
)

####################
# misc demo programs

cc_binary(
    name = "lookup_address",
    visibility = ["//visibility:public"],
    srcs = ["misc/lookup_address.cpp"],
    deps = [":reactor"],
)

cc_binary(
    name = "echo_client",
    visibility = ["//visibility:public"],
    srcs = ["misc/echo_client.cpp"],
    deps = [
        ":reactor_based_client",
        ":simple_tcp_service",
    ],
)

cc_binary(
    name = "echo_server",
    visibility = ["//visibility:public"],
    srcs = ["misc/echo_server.cpp"],
    deps = [
        ":reactor",
        "//:jmg_server_main",
        ":reactor_based_server",
        ":simple_tcp_service",
    ],
)
