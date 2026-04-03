load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

################################################################################
# old version of jmgc - for reference
################################################################################

cc_binary(
    name = "jmgc_old",
    srcs = [
        "jmg_spec.cpp",
        "jmg_spec.h",
        "jmgc_old.cpp",
        "quickfix_spec.cpp",
        "quickfix_spec.h",
        "spec_util.cpp",
        "spec_util.h",
    ],
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "//:jmg",
    ],
)
