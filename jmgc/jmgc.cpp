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

/**
 * compiler for Jormungandr field and message definitions
 */

#include <array>
#include <filesystem>
#include <iostream>

#include <ctre.hpp>

#include "jmg/cmdline.h"
#include "jmg/meta.h"
#include "jmg/util.h"

#include "jmgc_parser.h"
#include "jmgc_spec.h"

#include "cbe_spec.h"
#include "proto_spec.h"
#include "protoc_spec.h"
#include "yaml_spec.h"

using namespace jmg;
using namespace jmg::cmdline;
using namespace jmgc;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace fs = std::filesystem;

namespace
{
// interface definition language formats understood by jmgc
constexpr string_view kIdlJmg = "jmg"sv;
constexpr string_view kIdlQuickfix = "quickfix"sv;
constexpr auto kIdlTypes = array{kIdlJmg, kIdlQuickfix};
const auto kSupportIdlTypes = str_join(kIdlTypes, ", "sv);

// encodings that jmgc can generate wrappers for
constexpr char kWrapCbe[] = "cbe";
constexpr char kWrapYaml[] = "yaml";
constexpr char kWrapProto[] = "proto";
constexpr char kWrapFix[] = "fix";
constexpr auto kWrapperTypes = array{kWrapCbe, kWrapYaml, kWrapProto, kWrapFix};

// external IDL formats that jmgc can generate from JMG IDL
constexpr char kTgtProto[] = "protoc";
constexpr auto kTgtTypes = array{kTgtProto};

using OutputDir = NamedStringParam<"o", "output directory", Optional>;
using SrcIdl = NamedStringParam<"idl", "source IDL type to parse", Required>;
using WrapCbeFlag = NamedFlag<kWrapCbe, "generate CBE wrapper(s)">;
using WrapYamlFlag = NamedFlag<kWrapYaml, "generate YAML wrapper(s)">;
using WrapProtoFlag = NamedFlag<kWrapProto, "generate protobuf wrapper(s)">;
using WrapFixFlag = NamedFlag<kWrapFix, "generate FIX wrapper(s)">;
using TgtProto = NamedFlag<kTgtProto, "generate .proto for protoc">;
using SrcFile = PosnStringParam<"src_file", "path to IDL file">;

using CmdLine = CmdLineArgs<OutputDir,
                            SrcIdl,
                            WrapCbeFlag,
                            WrapYamlFlag,
                            WrapProtoFlag,
                            WrapFixFlag,
                            TgtProto,
                            SrcFile>;

} // namespace

int main(const int argc, const char** argv) {
  try {
    const auto cmdline = CmdLine(argc, argv);

    // sanity check source file
    const auto src_file = fs::path(jmg::get<SrcFile>(cmdline));
    JMG_ENFORCE(fs::exists(src_file), "argument source file [",
                src_file.native(), "] does not exist");
    const auto idl = jmg::get<SrcIdl>(cmdline);
    if (kIdlJmg == idl) {
      JMG_ENFORCE((".yaml" == src_file.extension())
                    || (".yml" == src_file.extension()),
                  "invalid source file path [", src_file.native(),
                  "] JMG IDL must be written in YAML");
    }
    else {
      JMG_ENFORCE(kIdlQuickfix == idl, "invalid unknown IDL type [", idl,
                  "] must be one of [", kSupportIdlTypes, "]");
      JMG_ENFORCE(".xml" == src_file.extension(), "invalid source file path [",
                  src_file.native(), "] QuickFIX IDL must be written in XML");
    }

    jmgc::JmgcYamlSpecMgr specMgr;

    size_t tgtCount = 0;
    if (kIdlJmg == idl) {
      if (jmg::get<WrapYamlFlag>(cmdline)) {
        cerr << "emitting YAML wrapper output\n";
        specMgr.add(make_unique<jmgc::YamlYamlSpec>());
        ++tgtCount;
      }
      if (jmg::get<WrapCbeFlag>(cmdline)) {
        cerr << "emitting CBE wrapper output\n";
        specMgr.add(make_unique<jmgc::CbeYamlSpec>());
        ++tgtCount;
      }
      if (jmg::get<WrapProtoFlag>(cmdline)) {
        cerr << "emitting protobuf wrapper output\n";
        specMgr.add(make_unique<jmgc::ProtoYamlSpec>());
        ++tgtCount;
      }
      if (jmg::get<TgtProto>(cmdline)) {
        cerr << "emitting protoc output\n";
        specMgr.add(make_unique<jmgc::ProtocYamlSpec>());
        ++tgtCount;
      }
      JMG_ENFORCE(tgtCount > 0, "no output encodings specified");
      jmgc::yaml::processYamlFile(src_file.native(), specMgr);
    }
    else { throw logic_error("non-YAML IDL file currently not supported"); }

    const auto output_dir = jmg::try_get<OutputDir>(cmdline);
    if (output_dir) { specMgr.emit(*output_dir); }
    else {
      JMG_ENFORCE(1 == tgtCount, "no more than 1 encoding may be generated "
                                 "when sending output to STDOUT");
      specMgr.emit(cout);
    }

    return EXIT_SUCCESS;
  }
  JMG_SINK_ALL_EXCEPTIONS_TO_STDERR("top level")
  return EXIT_FAILURE;
}
