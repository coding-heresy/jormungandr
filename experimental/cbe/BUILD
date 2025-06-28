
cc_library(
    name = 'cbe',
    visibility = ['//visibility:public'],
    srcs = ['cbe.cpp'],
    hdrs = ['cbe.h'],
    include_prefix = 'cbe',
    deps = ['//:jmg'],
)

cc_test(
    name = 'test_cbe',
    srcs = ['test_cbe.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        ':cbe',
    ],
)
