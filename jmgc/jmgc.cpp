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
#include <iostream>

#include "jmg/cmdline.h"
#include "jmg/meta.h"
#include "jmg/util.h"

#include "jmg_spec.h"
#include "quickfix_spec.h"

using namespace jmg;
using namespace std;

constexpr char kJmgCbeFlag[] = "JMG-CBE";
constexpr char kJmgYamlFlag[] = "JMG-YAML";
constexpr char kFixFlag[] = "FIX";
constexpr auto kSupportedFlags = array{kJmgCbeFlag, kJmgYamlFlag, kFixFlag};

const auto kSupportedFlagsStr = str_join(kSupportedFlags, ", "sv);

// TODO(bd) cmdline needs the concept of a union
using JmgYamlFlag = NamedFlag<
  kJmgYamlFlag,
  "file format is JMG, file type is YAML, generated encoding is YAML">;
using JmgCbeFlag =
  NamedFlag<kJmgCbeFlag,
            "file format is JMG, file type is YAML, generated encoding is CBE">;
using FixFlag =
  NamedFlag<kFixFlag, "file format is QuickFIX protocol, file type is XML">;

using SrcFile =
  PosnParam<string, "src_file", "the file to read source definitions from">;

int main(const int argc, const char** argv) {
  try {
    using CmdLine = CmdLineArgs<JmgYamlFlag, JmgCbeFlag, FixFlag, SrcFile>;
    const auto cmdline = CmdLine(argc, argv);

    if (jmg::get<FixFlag>(cmdline)) {
      JMG_ENFORCE_CMDLINE(!jmg::get<JmgYamlFlag>(cmdline)
                            && !jmg::get<JmgCbeFlag>(cmdline),
                          cmdline.usage(str_cat("at most one of [",
                                                kSupportedFlagsStr,
                                                "] may be specified")));
      // generate header file for FIX specification
      quickfix_spec::process(jmg::get<SrcFile>(cmdline));
    }
    else if (jmg::get<JmgCbeFlag>(cmdline)) {
      JMG_ENFORCE_CMDLINE(!jmg::get<JmgYamlFlag>(cmdline),
                          cmdline.usage(str_cat("at most one of [",
                                                kSupportedFlagsStr,
                                                "] may be specified")));
      // generate header file for CBE specification
      jmg_cbe_spec::process(jmg::get<SrcFile>(cmdline));
    }
    else {
      // must be -JMG-YAML
      JMG_ENFORCE_CMDLINE(jmg::get<JmgYamlFlag>(cmdline),
                          cmdline.usage(str_cat("at least one of [",
                                                kSupportedFlagsStr,
                                                "] must be specified")));
      // generate header file for YAML specification
      jmg_yml_spec::process(jmg::get<SrcFile>(cmdline));
    }
    return EXIT_SUCCESS;
  }
  catch (const CmdLineError& e) {
    cerr << e.what() << "\n";
  }
  JMG_SINK_ALL_EXCEPTIONS("top level")
  return EXIT_FAILURE;
}
