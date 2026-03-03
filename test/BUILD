load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_build_error//lang/cc:defs.bzl", "cc_build_error_test")
load("@rules_build_error//matcher:defs.bzl", "matcher")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "test_array_proxy",
    size = "small",
    srcs = ["test_array_proxy.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_cbe",
    size = "small",
    srcs = ["test_cbe.cpp"],
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_cmdline",
    size = "small",
    srcs = ["test_cmdline.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_conversion",
    size = "small",
    srcs = ["test_conversion.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_field",
    size = "small",
    srcs = ["test_field.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_file_util",
    size = "small",
    srcs = ["test_file_util.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_future",
    size = "small",
    srcs = ["test_future.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_ip_endpoint",
    size = "small",
    srcs = ["test_ip_endpoint.cpp"],
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_meta",
    size = "small",
    srcs = ["test_meta.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    srcs = ["data/test.proto"],
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

cc_proto_library(
    name = "test_proto_cc",
    deps = [":test_proto"],
)

cc_test(
    name = "test_protobuf",
    size = "small",
    srcs = ["test_protobuf.cpp"],
    linkstatic = True,
    deps = [
        ":test_proto_cc",
        "//:jmg",
        "@com_google_googletest//:gtest_main",
        "@protobuf",
    ],
)

cc_test(
    name = "test_object",
    size = "small",
    srcs = ["test_object.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_ptree",
    size = "small",
    srcs = ["test_ptree.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@boost.property_tree",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_random",
    size = "small",
    srcs = ["test_random.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_safe_types",
    size = "small",
    srcs = ["test_safe_types.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_types",
    size = "small",
    srcs = ["test_types.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_native",
    size = "small",
    srcs = ["test_native.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_union",
    size = "small",
    srcs = ["test_union.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_util",
    size = "small",
    srcs = ["test_util.cpp"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_yaml",
    size = "small",
    srcs = ["test_yaml.cpp"],
    data = ["data/experimental.yaml"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@bazel_tools//tools/cpp/runfiles",
        "@com_google_googletest//:gtest_main",
    ],
)

genrule(
    name = "fix_42_header",
    srcs = [
        "//jmgc",
        "@quickfix//:fix_42_spec",
    ],
    outs = ["quickfix_42.h"],
    # TODO(bd) run this command under control of "setarch $$(uname -m) -R"
    # for thread sanitizer builds?
    cmd = "$(location //jmgc:jmgc) -FIX $(location @quickfix//:fix_42_spec) > $@",
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test_quickfix",
    size = "small",
    srcs = [
        "test_quickfix.cpp",
        ":fix_42_header",
    ],
    tags = ["manual"],
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

genrule(
    name = "jmg_yaml_example_header",
    srcs = [
        "//jmgc:jmgc_old",
        "data/example.yaml",
    ],
    outs = ["jmg_yaml_example.h"],
    cmd = "$(location //jmgc:jmgc_old) -JMG-YAML $(location data/example.yaml) > $@",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test_jmg_yaml_example",
    size = "small",
    srcs = [
        "test_jmg_example.cpp",
        ":jmg_yaml_example_header",
    ],
    defines = ["YAML_ENCODING_TEST"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

genrule(
    name = "jmg_cbe_example_header",
    srcs = [
        "//jmgc:jmgc_old",
        "data/example.yaml",
    ],
    outs = ["jmg_cbe_example.h"],
    cmd = "$(location //jmgc:jmgc_old) -JMG-CBE $(location data/example.yaml) > $@",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test_jmg_cbe_example",
    size = "small",
    srcs = [
        "test_jmg_example.cpp",
        ":jmg_cbe_example_header",
    ],
    defines = ["CBE_ENCODING_TEST"],
    linkstatic = True,
    deps = [
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

################################################################################
# test of jmg_server_main intended to be run manually
################################################################################

genrule(
    name = "jmg_protobuf_example_header",
    srcs = [
        "//jmgc:jmgc_old",
        "data/test_proto.yaml",
    ],
    outs = ["test_proto.h"],
    cmd = "$(location //jmgc:jmgc_old) -JMG-proto $(location data/test_proto.yaml) > $@",
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "test_server_main",
    srcs = [
        "test_server_main.cpp",
    ],
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "//:jmg_server_main",
    ],
)

################################################################################
# expected compile/build failure tests
################################################################################

cc_build_error_test(
    name = "missing_time_format_in_conversion_fails_to_compile",
    src = "test_conversion_compile_fails.cpp",
    compile_stderr = matcher.has_substr("static assertion failed: conversion between string and time point must have at least one extra argument for the format"),
    local_defines = ["VERIFY_MISSING_TIME_FORMAT_FAILS"],
    deps = ["//:jmg"],
)

cc_build_error_test(
    name = "multiple_time_formats_in_conversion_fails_to_compile",
    src = "test_conversion_compile_fails.cpp",
    compile_stderr = matcher.has_substr("static assertion failed: more or less than one format string was specified when converting between string and time point"),
    local_defines = ["VERIFY_MULTIPLE_TIME_FORMATS_FAIL"],
    deps = ["//:jmg"],
)

cc_build_error_test(
    name = "multiple_time_zones_in_conversion_fails_to_compile",
    src = "test_conversion_compile_fails.cpp",
    compile_stderr = matcher.has_substr("static assertion failed: more than one time zone was specified when converting between string and time point"),
    local_defines = ["VERIFY_MULTIPLE_TIME_ZONES_FAIL"],
    deps = ["//:jmg"],
)
