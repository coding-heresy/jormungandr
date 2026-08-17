load("@rules_python//python:defs.bzl", "py_test")
load("//experimental/reflexive:reflexive_bindings.bzl", "reflexive_library")

cc_library(
    name = "reflexive",
    hdrs = ["reflexive.h"],
    copts = [
        "-std=c++26",  # TODO(bd) remove, should be at top level
    ],
    deps = [
        "//:jmg_python",
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

####################
# test code below here

cc_library(
    name = "test_features",
    srcs = ["test_features.cpp"],
    hdrs = ["test_features.h"],
    deps = ["//:jmg"],
)

reflexive_library(
    name = "reflexive_features",
    srcs = ["reflexive_features.cpp"],
    deps = [
        ":reflexive",
        ":test_features",
    ],
)

py_test(
    name = "test_reflexive_features",
    srcs = ["test_reflexive_features.py"],
    data = [":reflexive_features.so"],
    imports = ["."],
    main = "test_reflexive_features.py",
)

################################################################################
# TODO(bd) remove

reflexive_library(
    name = "engines",
    srcs = ["engines.cpp"],
    deps = [":reflexive"],
)

py_test(
    name = "test_engines",
    srcs = ["test_engines.py"],
    data = [":engines.so"],
    imports = ["."],
    main = "test_engines.py",
)
