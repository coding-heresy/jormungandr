
cc_library(
    name = "jmg",
    visibility = ["//visibility:public"],
    hdrs = [
        "include/jmg/array_proxy.h",
        "include/jmg/cmdline.h",
        "include/jmg/conversion.h",
        "include/jmg/field.h",
        "include/jmg/file_util.h",
        "include/jmg/future.h",
        "include/jmg/meta.h",
        "include/jmg/object.h",
        "include/jmg/preprocessor.h",
        "include/jmg/random.h",
        "include/jmg/safe_types.h",
        "include/jmg/server.h",
        "include/jmg/system.h",
        "include/jmg/types.h",
        "include/jmg/union.h",
        "include/jmg/util.h",
        # jmg language object types below here
        "include/jmg/cbe/cbe.h",
        "include/jmg/ptree/ptree.h",
        "include/jmg/quickfix/quickfix.h",
        "include/jmg/native/native.h",
        "include/jmg/yaml/yaml.h",
    ],
    srcs = [
        "src/cbe.cpp",
        "src/server.cpp",
        "src/system.cpp",
    ],
    includes = ["include"],
    linkstatic = True,
    deps = [
        "@boost.date_time",
        "@boost.property_tree",
        "@com_google_abseil//absl/container:btree",
        "@com_google_abseil//absl/container:flat_hash_map",
        "@com_google_abseil//absl/container:flat_hash_set",
        "@com_google_abseil//absl/time:time",
        "@ericniebler_meta//:meta",
        "@doom_strong_type//:strong_type",
        "@liburing",
        "@yaml-cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "jmg_server_main",
    visibility = ["//visibility:public"],
    srcs = [
        "src/server_main.cpp",
    ],
    linkstatic = True,
    deps = [
        ":jmg",
    ],
)
