/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Brian Davis <brian8702@sbcglobal.net>
 *
 */

#include "jmg/test_util.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "tools/cpp/runfiles/runfiles.h"

#include "jmg/preprocessor.h"
#include "jmg/util.h"

// using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace fs = std::filesystem;

using bazel::tools::cpp::runfiles::Runfiles;

namespace
{

using RunfilesPtr = unique_ptr<Runfiles>;

/**
 * meyers singleton that gets a reference to the initialized Runtiles object
 */
Runfiles& runfiles() {
  static auto instance = []() -> RunfilesPtr {
    string err_msg;
    auto rslt = RunfilesPtr(Runfiles::CreateForTest(&err_msg));
    // TODO(bd) figure out why the usual application of JMG_ENFORCE
    // causes an error message about using err_msg before deduction of
    // 'auto'
#if defined(JMG_ENFORCE_WORKS_CORRECTLY_HERE)
    JMG_ENFORCE(rslt, "unable to load bazel runfiles: "sv, err_msg);
#else
    if (!rslt) {
      const auto msg =
        jmg::str_cat("unable to load bazel runfiles: "sv, err_msg);
      JMG_THROW_EXCEPTION(std::runtime_error, msg);
    }
#endif
    return rslt;
  }();
  return *instance;
}

/**
 * meyers singleton that gets a reference to the string that can be
 * used to bridge the gap between a relative path computed by a
 * starlark location/rlocation and the absolute path retrieved by
 * Runfiles::Rlocation
 *
 * NOTE: this description may not be 100% accurate...
 */
const string& current_repo() {
  static const auto instance = []() -> string {
    auto rslt = string(BAZEL_CURRENT_REPOSITORY);
    if (rslt.empty()) { rslt = "_main"s; }
    return rslt;
  }();
  return instance;
}

} // namespace

namespace jmg
{

/**
 * get the absolute runfiles/sandbox path to a relative path computed
 * by a starlark location/rlocation
 */
string rlocation(const string_view relative_path) {
  return runfiles().Rlocation(str_cat(current_repo(), "/"sv, relative_path));
}

/**
 * get a reference to an initialized python interpreter that can be
 * used to test C++ code that interacts with python
 */
PyConfig& python_cfg() {
  static auto instance = []() -> PyConfig {
    const auto python_interpreter =
      rlocation(string_view(PYTHON_INTERPRETER_RLOCATION));
    JMG_ENFORCE(!python_interpreter.empty(),
                "unable to find hermetic python toolchain in runfiles");
    // NOTE: the hermetic python toolchain home directory is the
    // directory before the one containing the python interpreter
    const auto python_home =
      fs::path(python_interpreter).parent_path().parent_path();
    JMG_ENFORCE(!python_home.native().empty(),
                "unable to find hermetic python toolchain in runfiles");
    PyConfig rslt;
    PyConfig_InitIsolatedConfig(&rslt);
    PyConfig_SetBytesString(&rslt, &rslt.home, python_home.c_str());
    return rslt;
  }();
  return instance;
}

} // namespace jmg
