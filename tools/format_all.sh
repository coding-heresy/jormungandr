#!/bin/bash

# TODO(bd) use a hermetic version of clang-format?

# format all C++ code using rules provided in .clang-format
find . -type f -name '*.h' -o -name '*.cpp' | xargs clang-format -i 2>/dev/null
