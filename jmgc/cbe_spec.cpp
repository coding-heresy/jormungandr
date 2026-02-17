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

#include "cbe_spec.h"

using namespace jmg;
using namespace std;
using namespace std::string_view_literals;

namespace jmgc
{

string CbeYamlSpec::tgtFileName() const {
  JMG_ENFORCE_USING(
    logic_error, pred(pkg_),
    "requested target file name before input file was processed");
  return str_cat(snakeCaseToCamelCase(jmg::get<Name>(*pkg_)), ".cbe.h");
}

string_view CbeYamlSpec::encodingHeaderFileName() const {
  return "jmg/cbe/cbe.h"sv;
}

string_view CbeYamlSpec::encodingNamespace() const { return "jmg::cbe"sv; }

string_view CbeYamlSpec::encodingFieldDef() const { return "Field"sv; }

void CbeYamlSpec::enrichFld(ostream& strm, const ObjGrpFld& fld_def) const {
  const auto id = jmg::try_get<CbeId>(fld_def);
  JMG_ENFORCE(pred(id), "no CBE field ID was provided for field [",
              jmg::get<Name>(fld_def), "]");
  strm << ", " << *id << "U";
}

} // namespace jmgc
