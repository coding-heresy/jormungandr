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

#include <iostream>

#include "jmg/cmdline.h"
#include "jmg/meta.h"

#include "jmg_spec.h"
#include "quickfix_spec.h"

using namespace jmg;
using namespace std;

using JmgFlag =
  NamedParam<bool, "JMG", "file format is JMG, file type is YAML", Required>;
using FixFlag =
  NamedParam<bool, "FIX", "file format is FIX protocol, file type is XML", Required>;
using SrcFile =
  PosnParam<string, "src_file", "the file to read source definitions from">;

int main(const int argc, const char** argv) {
  try {
    using CmdLine = CmdLineArgs<FixFlag, JmgFlag, SrcFile>;
    const auto cmdline = CmdLine(argc, argv);

    if (get<FixFlag>(cmdline)) {
      // generate header file for FIX specification
      JMG_ENFORCE_USING(CmdLineError, !get<JmgFlag>(cmdline),
                        "at most one of [-JMG, -FIX] may be specified");
      quickfix_spec::process(get<SrcFile>(cmdline));
    }
    else {
      JMG_ENFORCE_USING(CmdLineError, get<JmgFlag>(cmdline),
                        "at least one of [-JMG, -FIX] must be specified");
      jmg_spec::process(get<SrcFile>(cmdline));
    }
    return EXIT_SUCCESS;
  }
  catch (const CmdLineError& e) {
    cerr << "ERROR: " << e.what() << "\n";
  }
  catch (const exception& e) {
    cerr << "exception: " << e.what() << "\n";
  }
  catch (...) {
    cerr << "unexpected exception type [" << current_exception_type_name()
         << "]\n";
  }
  return EXIT_FAILURE;
}
