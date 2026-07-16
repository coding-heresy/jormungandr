# TODO(bd) rewrite this to take only the name of the library and
# enforce that the name of the library is the same as the name of the
# source file, which should then be used as part of a mechanism to
# ensure that the name of the .so file matches the name of the python
# module
def reflexive_library(
        name,
        srcs = [],
        hdrs = [],
        deps = [],
        copts = [],
        **kwargs):
    """Generates both a static library and a Python-bound shared object target.

    Args:
        name: The base name string.
              The shared object will be named `name + ".so"`.
        srcs: The list of C++ source files.
        hdrs: The list of header files.
        deps: Dependencies required for the core C++ logic.
        **kwargs: Propagated attributes like visibility or tags.
    """

    # static library with no python bindings
    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = copts + ["-Wsfinae-incomplete=2"],
        deps = deps,
        **kwargs
    )

    # shared object library has python bindings
    native.cc_binary(
        name = name + ".so",
        # put headers and sources into srcs to compile this as a fresh
        # translation unit, no need to have separate hdrs because this
        # target should only be used with python
        srcs = srcs + hdrs,
        local_defines = ["JMG_ENABLE_PYTHON_BINDINGS"],
        linkshared = True,
        copts = copts + ["-Wsfinae-incomplete=2"],
        deps = deps + [
            "@rules_python//python/cc:current_py_cc_libs",
        ],
        **kwargs
    )
