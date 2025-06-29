
cc_library(
    name = 'control_blocks',
    visibility = ['//visibility:public'],
    hdrs = ['control_blocks.h'],
    deps = ['//:jmg'],
)

cc_test(
    name = 'test_control_blocks',
    srcs = ['test_control_blocks.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        ':control_blocks'
    ],
)

cc_library(
    name = 'reactor',
    visibility = ['//visibility:public'],
    srcs = [
        'reactor.cpp',
    ],
    hdrs = [
        'fiber.h',
        'reactor.h'
    ],
    deps = [
        ':control_blocks',
    ],
)

cc_test(
    name = 'test_reactor',
    srcs = ['test_reactor.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        ':reactor'
    ],
    tags = ['manual'],
)
