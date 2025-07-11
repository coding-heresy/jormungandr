
cc_test(
    name = 'test_array_proxy',
    srcs = ['test_array_proxy.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_cbe',
    srcs = ['test_cbe.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    size = 'small',
)

cc_test(
    name = 'test_cmdline',
    srcs = ['test_cmdline.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_conversion',
    srcs = ['test_conversion.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_field',
    srcs = ['test_field.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_file_util',
    srcs = ['test_file_util.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_future',
    srcs = ['test_future.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_meta',
    srcs = ['test_meta.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_object',
    srcs = ['test_object.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_ptree',
    srcs = ['test_ptree.cpp'],
    deps = [
        '@boost//:property_tree',
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_random',
    srcs = ['test_random.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_safe_types',
    srcs = ['test_safe_types.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_types',
    srcs = ['test_types.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_native',
    srcs = ['test_native.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_util',
    srcs = ['test_util.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_test(
    name = 'test_yaml',
    srcs = ['test_yaml.cpp'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

genrule(
    name = 'fix_42_header',
    visibility = ['//visibility:public'],
    srcs = [
        '//jmgc:jmgc',
        '@quickfix//:fix_42_spec',
    ],
    outs = ['quickfix_42.h'],
    cmd = '$(location //jmgc:jmgc) -FIX $(location @quickfix//:fix_42_spec) > $@',
    tags = ['manual'],
)

cc_test(
    name = 'test_quickfix',
    srcs = [
        ':fix_42_header',
        'test_quickfix.cpp'
    ],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    tags = ['manual'],
    size = 'small',
)

genrule(
    name = 'jmg_yaml_example_header',
    visibility = ['//visibility:public'],
    srcs = [
        '//jmgc:jmgc',
        'data/example.yaml',
    ],
    outs = ['jmg_yaml_example.h'],
    cmd = '$(location //jmgc:jmgc) -JMG-YAML $(location data/example.yaml) > $@',
)

cc_test(
    name = 'test_jmg_yaml_example',
    srcs = [
        ':jmg_yaml_example_header',
        'test_jmg_example.cpp'
    ],
    defines = ['YAML_ENCODING_TEST'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

genrule(
    name = 'jmg_cbe_example_header',
    visibility = ['//visibility:public'],
    srcs = [
        '//jmgc:jmgc',
        'data/example.yaml',
    ],
    outs = ['jmg_cbe_example.h'],
    cmd = '$(location //jmgc:jmgc) -JMG-CBE $(location data/example.yaml) > $@',
)

cc_test(
    name = 'test_jmg_cbe_example',
    srcs = [
        ':jmg_cbe_example_header',
        'test_jmg_example.cpp'
    ],
    defines = ['CBE_ENCODING_TEST'],
    deps = [
        '@com_google_googletest//:gtest_main',
        '//:jmg',
    ],
    linkstatic = True,
    size = 'small',
)

cc_binary(
    name = 'test_server_main',
    visibility = ['//visibility:public'],
    srcs = [
        'test_server_main.cpp',
    ],
    linkstatic = True,
    deps = [
        '//:jmg_server_main',
    ],
)
