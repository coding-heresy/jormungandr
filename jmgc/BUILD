load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:jmgc.bzl", "cc_jmg_library", "proto_jmg_library")

cc_binary(
    name = "jmgc",
    srcs = [
        "cbe_spec.cpp",
        "cbe_spec.h",
        "jmgc.cpp",
        "jmgc_parser.cpp",
        "jmgc_parser.h",
        "jmgc_spec.cpp",
        "jmgc_spec.h",
        "proto_spec.cpp",
        "proto_spec.h",
        "protoc_spec.cpp",
        "protoc_spec.h",
        "yaml_spec.cpp",
        "yaml_spec.h",
    ],
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "//:jmg",
        "@jinja2cpp",
    ],
)

####################
# JMG IDL YAML description file used to test jmgc implementations

filegroup(
    name = "test_jmgc_src",
    srcs = ["test_jmgc.yaml"],
    visibility = ["//visibility:private"],
)

####################
# test generating and using YAML wrapper

# c++ JMG wrapper from IDL spec
cc_jmg_library(
    name = "test_jmgc_yaml_wrap_cc",
    src = ":test_jmgc_src",
    encoding = "yaml",
    visibility = ["//visibility:private"],
)

# TODO(bd) this test is currently failing due to incompatibiities in handling array fields in the YAML encoding vs other encodings
cc_test(
    name = "test_jmgc_yaml",
    size = "small",
    srcs = ["test_jmgc_yaml.cpp"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_jmgc_yaml_wrap_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

####################
# test generating .proto IDL from JMG IDL along with protobuf wrapper generated from the same IDL file

# .proto file from IDL spec
proto_jmg_library(
    name = "test_jmgc_src_proto",
    src = ":test_jmgc_src",
    visibility = ["//visibility:private"],
    deps = ["@protobuf//:timestamp_proto"],
)

# c++ bindings for .proto
cc_proto_library(
    name = "test_jmgc_proto_cc",
    visibility = ["//visibility:private"],
    deps = [":test_jmgc_src_proto"],
)

# c++ JMG wrapper from IDL spec
cc_jmg_library(
    name = "test_jmgc_proto_wrap_cc",
    src = ":test_jmgc_src",
    encoding = "proto",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "test_jmgc_any_hdr",
    hdrs = ["test_jmgc_any.h"],
    visibility = ["//visibility:private"],
)

cc_test(
    name = "test_jmgc_proto",
    size = "small",
    srcs = [
        "test_jmgc_any.cpp",
        "test_jmgc_proto.cpp",
    ],
    defines = ["JMG_TEST_PROTOBUF"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_jmgc_any_hdr",
        ":test_jmgc_proto_cc",
        ":test_jmgc_proto_wrap_cc",
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)

####################
# test generating and using CBE wrapper

# c++ JMG wrapper from IDL spec
cc_jmg_library(
    name = "test_jmgc_cbe_wrap_cc",
    src = ":test_jmgc_src",
    encoding = "cbe",
    visibility = ["//visibility:private"],
)

cc_test(
    name = "test_jmgc_cbe",
    size = "small",
    srcs = [
        "test_jmgc_any.cpp",
        "test_jmgc_cbe.cpp",
    ],
    defines = ["JMG_TEST_CBE"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_jmgc_any_hdr",
        ":test_jmgc_cbe_wrap_cc",
        "//:jmg",
        "@com_google_googletest//:gtest_main",
    ],
)
