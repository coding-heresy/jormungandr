/** -*- mode: c++ -*-
 *
 * Copyright (C) 2025 Brian Davis
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
 * demo of Jormungandr command line argument handling using
 * `jmg::CmdLineArgs`
 */

#include <iostream>

#include "jmg/cmdline.h"

using namespace jmg;
using namespace std;

namespace
{
using Flag = cmdline::NamedFlag<"flag", "a flag">;
using OptNamedStr =
  cmdline::NamedStringParam<"opt_str", "an optional string", Optional>;
using NamedInt =
  cmdline::NamedParam<int, "required_int", "a required integer", Required>;
using PosnDbl =
  cmdline::PosnParam<double,
                     "posn_double",
                     "the first position param, which is required">;
using OptPosnFlt =
  cmdline::PosnParam<float,
                     "opt_posn_float",
                     "the second position param, which is optional",
                     Optional>;
using CmdLine =
  cmdline::CmdLineArgs<Flag, OptNamedStr, NamedInt, PosnDbl, OptPosnFlt>;
} // namespace

int main(const int argc, const char** argv) {
  try {
    const auto cmdline = CmdLine(argc, argv);

    // check for flag
    if (jmg::get<Flag>(cmdline)) { cout << "flag was set\n"; }
    else { cout << "flag was not set\n"; }

    // check for optional named string
    if (const auto opt_named_str = jmg::try_get<OptNamedStr>(cmdline);
        opt_named_str) {
      cout << "optional named string was provided with value ["
           << *opt_named_str << "]\n";
    }
    else { cout << "optional named string was not provided\n"; }

    // log required named int
    cout << "required named int had value [" << jmg::get<NamedInt>(cmdline)
         << "]\n";

    // log required positional double
    cout << "required positional double had value ["
         << jmg::get<PosnDbl>(cmdline) << "]\n";

    // check for optional positional float
    if (const auto opt_posn_flt = jmg::try_get<OptPosnFlt>(cmdline);
        opt_posn_flt) {
      cout << "optional positional float was provided with value ["
           << *opt_posn_flt << "]\n";
    }
    else { cout << "optional positional float was not provided\n"; }
  }
  catch (const cmdline::CmdLineError& e) {
    cerr << e.what() << "\n";
  }
  JMG_SINK_ALL_EXCEPTIONS("top level")
}
