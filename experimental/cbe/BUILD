
cc_library(
    name = 'cbe',
    srcs = ['cbe.cpp'],
    hdrs = ['cbe.h'],
    linkstatic = True,
    alwayslink = True,
    deps = ['//:jmg'],
)

cc_test(
    name = 'test_cbe',
    srcs = ['test_cbe.cpp'],
    linkstatic = True,
    deps = [
        '@com_google_googletest//:gtest_main',
        ':cbe',
    ],
)
