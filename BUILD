
cc_library(
    name = 'jmg',
    visibility = ['//visibility:public'],
    hdrs = [
        'include/jmg/array_proxy.h',
        'include/jmg/cmdline.h',
        'include/jmg/conversion.h',
        'include/jmg/field.h',
        'include/jmg/file_util.h',
        'include/jmg/meta.h',
        'include/jmg/object.h',
        'include/jmg/preprocessor.h',
        'include/jmg/ptree/ptree.h',
        'include/jmg/quickfix/quickfix.h',
        'include/jmg/random.h',
        'include/jmg/safe_types.h',
        'include/jmg/server.h',
        'include/jmg/system.h',
        'include/jmg/tuple_object/tuple_object.h',
        'include/jmg/types.h',
        'include/jmg/union.h',
        'include/jmg/util.h',
        'include/jmg/yaml/yaml.h',
    ],
    srcs = [
        'src/server.cpp',
    ],
    includes = ['include'],
    linkstatic = True,
    deps = [
        '@com_google_abseil//absl/container:btree',
        '@com_google_abseil//absl/container:flat_hash_map',
        '@com_google_abseil//absl/container:flat_hash_set',
        '@com_google_abseil//absl/time:time',
        '@ericniebler_meta//:meta',
        '@doom_strong_type//:strong_type',
        '@yaml-cpp//:yaml-cpp',
        '@boost//:property_tree',
    ],
)

cc_library(
    name = 'jmg_server_main',
    visibility = ['//visibility:public'],
    srcs = [
        'src/server_main.cpp',
    ],
    linkstatic = True,
    deps = [
        ':jmg',
    ],
)
