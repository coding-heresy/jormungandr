/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#include <filesystem>
#include <iostream>
#include <string>

#include <gmock/gmock.h>

#include "tools/cpp/runfiles/runfiles.h"

#include "jmg/preprocessor.h"
#include "jmg/util.h"

#include "jmgc_parser.h"
#include "jmgc_spec.h"

using namespace YAML;
using namespace jmg;
using namespace std;
using namespace std::string_literals;

namespace fs = std::filesystem;

namespace
{

template<typename... Args>
void sendOut(Args&&... args) {
  cout << str_cat(">>>>> ", std::forward<Args>(args)...) << endl;
}

class TestSpec : public jmgc::JmgcYamlSpecIfc {
public:
  void processPkg(const Node& jmg_pkg) override {
    sendOut("package name is [", jmg_pkg["name"].as<string>(), "]");
  }

  void processType(const Node& jmg_type) override {
    sendOut("type name is [", jmg_type["name"].as<string>(), "]");
  }

  void processObjFld(const string_view obj_name, const Node& jmg_fld) override {
    sendOut("object [", obj_name, "] has field [", jmg_fld["name"].as<string>(),
            "]");
  }

  void emit(ostream& strm) override {}
};

const auto kExperimentalFilePath = "_main/jmgc/experimental.yaml"s;

} // namespace

TEST(JmgcParserTests, SmokeTest) {
  using bazel::tools::cpp::runfiles::Runfiles;

  std::string err_msg;
  Runfiles* ptr = Runfiles::CreateForTest(&err_msg);
  if (!ptr) { FAIL() << "unable to provide runfiles: [" << err_msg << "]"; }
  if (!(err_msg.empty())) {
    FAIL() << "runfiles provision failed: [" << err_msg << "]";
  }
  auto& run_files = *ptr;

  const auto file_path = run_files.Rlocation(kExperimentalFilePath);
  JMG_ENFORCE(!file_path.empty(), "runfiles cannot find actual path for [",
              kExperimentalFilePath, "]");
  JMG_ENFORCE(fs::exists(fs::path(file_path)), "[", file_path, "] for [",
              kExperimentalFilePath, "] does not exist");

  auto spec = [&] -> jmgc::JmgcYamlSpecMgr {
    jmgc::JmgcYamlSpecMgr rslt;
    rslt.add(make_unique<jmgc::JmgYamlSpec>());
    return rslt;
  }();

  jmgc::yaml::processYamlFile(file_path, spec);
  cout << "============================================================\n";
  spec.emit(std::filesystem::path("/var/tmp"));
}
