load("@rules_cc//cc:cc_library.bzl", "cc_library")

# implementation of jmgc wrapper generation
def _jmgc_wrapper_gen_impl(ctx):
    src_yaml = ctx.file.src
    src_base = src_yaml.basename
    encoding = ctx.attr.encoding

    if not src_base.endswith(".yaml"):
        fail("source file [{}] must end with [.yaml]".format(src_base))
    tgt_base = src_base.removesuffix(".yaml")
    tgt_hdr = ctx.actions.declare_file("{}.{}.h".format(
        tgt_base,
        encoding
    ))

    print("generating header file [{}] for encoding [{}]".format(
        tgt_hdr.path,
        encoding,
    ))

    ctx.actions.run_shell(
        outputs = [tgt_hdr],
        inputs = [src_yaml],
        tools = [ctx.executable._tool],
        command = "$1 -idl jmg -$2 $3 > $4",
        arguments = [
            ctx.executable._tool.path,
            encoding,
            src_yaml.path,
            tgt_hdr.path
        ],
        mnemonic = "JmgcWrapperGen",
        progress_message = "generating %s from %s" % (
            tgt_hdr.short_path,
            src_yaml.short_path,
        ),
    )

    return [DefaultInfo(files = depset([tgt_hdr]))]

# rule for jmgc wrapper generation
_jmgc_wrapper_gen = rule(
    implementation = _jmgc_wrapper_gen_impl,
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True
        ),
        "encoding": attr.string(
            mandatory = True,
            # NOTE: new encodings must be added here
            values = ["cbe", "yaml", "proto"],
        ),
        "_tool": attr.label(
            default = Label("//jmgc:jmgc"),
            executable = True,
            cfg = "exec"
        ),
    },
)

# macro for jmgc wrapper generation
def cc_jmg_library(
    name,
    src,
    encoding,
    **kwargs
):
    """generate an encoding wrapper from a JMG IDL YAML file"""
    
    gen_target = "{}_gen".format(name)
    
    _jmgc_wrapper_gen(
        name = gen_target,
        src = src,
        encoding = encoding,
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name,
        hdrs = [":{}".format(gen_target)],
        includes = ["."],
        deps = ["//:jmg"],
        **kwargs
    )

################################################################################

# implementation of jmgc wrapper generation
def _jmgc_proto_gen_impl(ctx):
    src_yaml = ctx.file.src
    src_base = src_yaml.basename

    if not src_base.endswith(".yaml"):
        fail("source file [{}] must end with [.yaml]".format(src_base))
    tgt_base = src_base.removesuffix(".yaml")
    tgt_proto = ctx.actions.declare_file("{}.proto".format(tgt_base))

    print("generating protobuf IDL file [{}]".format(tgt_proto.path))

    ctx.actions.run_shell(
        outputs = [tgt_proto],
        inputs = [src_yaml],
        tools = [ctx.executable._tool],
        command = "$1 -idl jmg -protoc $2 > $3",
        arguments = [
            ctx.executable._tool.path,
            src_yaml.path,
            tgt_proto.path
        ],
        mnemonic = "JmgcProtoGen",
        progress_message = "generating %s from %s" % (
            tgt_proto.short_path,
            src_yaml.short_path,
        ),
    )

    return [DefaultInfo(files = depset([tgt_proto]))]

# rule for jmgc .proto generation
_jmgc_proto_gen = rule(
    implementation = _jmgc_proto_gen_impl,
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True
        ),
        "_tool": attr.label(
            default = Label("//jmgc:jmgc"),
            executable = True,
            cfg = "exec"
        ),
    },
)

# macro for jmgc .proto generation
def proto_jmg_library(
    name,
    src,
    **kwargs
):
    """generate a .proto file from a JMG IDL YAML file"""
    
    gen_target = "{}_gen".format(name)
    
    _jmgc_proto_gen(
        name = gen_target,
        src = src,
        visibility = ["//visibility:private"],
    )

    native.proto_library(
        name = name,
        srcs = [":{}".format(gen_target)],
        **kwargs
    )
