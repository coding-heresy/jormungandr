
####################
# control blocks

cc_library(
    name = "control_blocks",
    hdrs = ["control_blocks.h"],
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
    hdrs = [
        "uring.h",
        "util.h",
    ],
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
        "thread_pool.h",
    ],
    hdrs = [
        "fiber.h",
        "reactor.h"
    ],
    deps = [
        # TODO(bd) figure out how to make boost.asio work with shadow
        # stacks or try a different thread pool implementation
        # "@boost.asio",
        "@bs_thread-pool//:thread_pool",
        ":control_blocks",
        ":uring",
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
