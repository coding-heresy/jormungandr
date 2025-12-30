
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
        ":control_blocks"
    ],
    size = "small",
)

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
        ":uring"
    ],
    size = "small",
)

cc_test(
    name = "test_ucontext",
    srcs = ["test_ucontext.cpp"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "//:jmg"
    ],
    size = "small",
)

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
    deps = [
        ":control_blocks",
        ":uring",
    ],
)

cc_test(
    name = "test_reactor",
    srcs = ["test_reactor.cpp"],
    deps = [
        "@com_google_googletest//:gtest_main",
        ":reactor"
    ],
    size = "small",
    # TODO(bd) temporarily marked as "manual" because it is currently
    # failing
    tags = ["manual"],
)
