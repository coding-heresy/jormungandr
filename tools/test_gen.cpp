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

/**
 * simple program that uses the c++ version of jinja2 templates to automatically
 * generate simple bazel cc_test rules for all source files in a directory as
 * part of the process of adding native bazel builds to existing repos
 *
 * command line arguments consist of the required path to the directory to build
 * test cases for plus zero or more bazel labels that will be added as
 * dependencies to each generated cc_test rule
 */

#include <exception>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>

#include "jmg/meta.h"

#include "jinja2cpp/template.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;
namespace fs = std::filesystem;
namespace vws = std::views;

const auto test_rule_tmpl = R"(
cc_test(
    name = "{{test_name}}",
    size = "small",
    srcs = ["{{test_src}}"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_googletest//:gtest_main",
{% for dep in deps -%}
        "{{ dep }}",
{% endfor -%}
    ],
)
)"s;

/**
 * return true if the argument directory entry represents a c++ source file
 */
bool isTestSrc(const fs::directory_entry& entry) {
  if (const auto str_path = entry.path().string();
      fs::is_regular_file(entry)
      && (str_path.ends_with(".cpp") || str_path.ends_with(".cc"))
      // TODO(bd) add more legal extensions?
  ) {
    return true;
  }
  return false;
}

void usage(const std::string_view executable) {
  cerr << "usage: " << executable << " <path to test directory> "
       << "[dependency rule name ...]" << endl;
  exit(EXIT_FAILURE);
}

int main(int argc, const char* argv[]) {
  try {
    if (argc < 2) {
      cerr << "ERROR: must provide a path to a directory on the command line"
           << endl;
      usage(argv[0]);
    }
    const auto test_dir = fs::path(argv[1]);
    if (!fs::is_directory(test_dir)) {
      cerr << "ERROR: first argument [" << argv[1]
           << "] is not a path that references a directory" << endl;
      usage(argv[0]);
    }
    auto deps = [&] {
      jinja2::ValuesList rslt;
      for (int idx = 2; idx < argc; ++idx) { rslt.push_back(argv[idx]); }
      return rslt;
    }();
    auto tmpl = [&] {
      jinja2::Template rslt;
      if (!rslt.Load(test_rule_tmpl)) {
        cerr << "ERROR: unable to load template [\n"
             << test_rule_tmpl << "\n]" << endl;
      }
      return rslt;
    }();
    auto srcs =
      fs::recursive_directory_iterator(test_dir) | vws::filter(isTestSrc);
    for (const auto& src : srcs) {
      auto subs =
        jinja2::ValuesMap{// NOTE: cc_test rule name will be the same as the name
                          // of the associated source file without its extension
                          {"test_name", src.path().stem()},
                          {"test_src", src.path().string()}};
      if (deps.size() > 0) { subs["deps"] = deps; }
      cout << tmpl.RenderAsString(subs).value();
    }
    return EXIT_SUCCESS;
  }
  catch (const fs::filesystem_error& e) {
    cerr << "caught filesystem error at top level: " << e.what() << endl;
  }
  catch (const exception& e) {
    cerr << "caught standard exception at top level: " << e.what() << endl;
  }
  catch (...) {
    cerr << "caught unexpected exception type ["
         << current_exception_type_name() << "] at top level" << endl;
  }
  return EXIT_FAILURE;
}
