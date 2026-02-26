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

#include "yaml_spec.h"

using namespace jmg;
using namespace std;
using namespace std::string_view_literals;

namespace jmgc
{

string YamlYamlSpec::tgtFileName() const {
  JMG_ENFORCE_USING(
    logic_error, pred(pkg_),
    "requested target file name before input file was processed");
  return str_cat(snakeCaseToCamelCase(jmg::get<Name>(*pkg_)), ".yaml.h");
}

string_view YamlYamlSpec::encodingHeaderFileName() const {
  return "jmg/yaml/yaml.h"sv;
}

std::string YamlYamlSpec::encodingObjDef() const {
  return "jmg::yaml::Object"s;
}

} // namespace jmgc
