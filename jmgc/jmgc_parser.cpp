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

#include <tuple>

#include "jmgc_parser.h"

#include "jmg/preprocessor.h"
#include "jmg/util.h"

using namespace YAML;
using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace vws = std::views;

namespace jmgc
{
namespace yaml
{

/**
 * get the string representation of a YAML node type
 */
string_view yamlNodeType(const YAML::NodeType::value yaml_type);

/**
 * get the string representation of the type of a YAML node
 */
string_view yamlNodeType(const NodeType::value yaml_type) {
  switch (yaml_type) {
    case NodeType::Null:
      return "null"sv;
    case NodeType::Scalar:
      return "scalar"sv;
    case NodeType::Sequence:
      return "sequence"sv;
    case NodeType::Map:
      return "map"sv;
    default:
      break;
  }
  return "<unknown>"s;
}

string_view yamlTypeOf(const Node& node) {
  if (!node.IsDefined()) { return "undefined"sv; }
  return yamlNodeType(node.Type());
}

// I do not generally recommend using macros in this way but sometimes complex
// code generation is the best tool

/**
 * macro that that emits text for forwarding a specific parameter pack
 */
#define JMG_FWD_DESC() std::forward<DescParts>(desc_parts)...

/**
 * macro that that emits text for converting a specific parameter pack into a
 * string
 */
#define JMG_STR_DESC() str_cat(JMG_FWD_DESC(), " ")

/**
 * function template that enforces the constraint that a YAML node must have a
 * certain type, throwing exception if the constraint is not satisfied
 */
template<typename... DescParts>
void enforceType(const Node& node,
                 const NodeType::value required_type,
                 DescParts&&... desc_parts) {
  JMG_ENFORCE(node.IsDefined(), JMG_STR_DESC(), "is not defined");
  JMG_ENFORCE(required_type == node.Type(), JMG_STR_DESC(),
              "is required to have type [", yamlNodeType(required_type),
              "] but has type [", yamlNodeType(node.Type()), "] instead");
}

/**
 * function template that unpacks a YAML node expected to be of a specific
 * structure
 *
 * in particular, the node must be a map with 1 entry consisting of a key and an
 * inner map
 *
 * @return a 2-tuple consisting of the key of the entry and the YAML node
 * containing the inner map
 */
template<typename... DescParts>
auto unpackSingleEntryMap(const Node& node, DescParts&&... desc_parts) {
  JMG_ENFORCE(node.IsDefined(), JMG_STR_DESC(), "is not defined");
  enforceType(node, NodeType::Map, JMG_FWD_DESC());
  JMG_ENFORCE(1 == node.size(), JMG_STR_DESC(), "has size [", node.size(),
              "] instead of required size [1]");
  auto itr = node.begin();
  const auto& name = itr->first.as<string>();
  const auto body = itr->second;
  return make_tuple(name, std::move(body));
}

/**
 * function template that converts the result of a call to unpackSingleEntryMap
 * into a new node by cloning the inner map and adding the original key to it as
 * an element keyed by the string "name"
 */
template<typename... DescParts>
Node makeObjNode(const Node& node, DescParts&&... desc_parts) {
  auto [name, body] = unpackSingleEntryMap(node, JMG_FWD_DESC());
  enforceType(body, NodeType::Map, "body of ", JMG_FWD_DESC());
  JMG_ENFORCE(!pred(body["name"]), "body of ", JMG_STR_DESC(),
              "has extraneous name field [", body["name"].template as<string>(),
              "]");
  // clone the body and set its name to the key value
  auto obj = Clone(body);
  obj["name"] = name;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////
// main "parsing" function
////////////////////////////////////////////////////////////////////////////////

void processYamlFile(const string_view file_path, jmgc::JmgcYamlSpecIfc& spec) {
  JMG_ENFORCE(file_path.ends_with(".yml") || file_path.ends_with(".yaml"),
              "encountered non-YAML file [", file_path,
              "] when attempting to process a JMG specification");
  const auto contents = LoadFile(string(file_path));
  const auto msg_prefix = str_cat("JMG format file [", file_path, "] ");
  enforceType(contents, NodeType::Map, msg_prefix, "top-level content");

  ////////////////////
  // process required 'package' section

  JMG_ENFORCE(
    pred(contents["package"]), msg_prefix,
    "is required to have a top-level 'package' section, but does not");
  spec.processPkg(contents["package"]);

  ////////////////////
  // process optional 'types' section, if present
  if (contents["types"]) {
    const auto& types = contents["types"];
    enforceType(types, NodeType::Sequence, "available 'types' section");
    for (auto [idx, entry] : vws::enumerate(types)) {
      spec.processType(makeObjNode(entry, "'types' section entry [", idx, "]"));
    }
  }

  ////////////////////
  // process optional 'groups' section, if present
  // TODO(bd) process groups

  ////////////////////
  // process optional 'objects' section, if present
  if (contents["objects"]) {
    const auto& objs = contents["objects"];
    enforceType(objs, NodeType::Sequence, "available 'objects' section");

#define JMG_OBJS_ENTRY() "'objects' section entry [", obj_idx, "]"
    for (auto [obj_idx, obj_entry] : vws::enumerate(objs)) {
      // each object has a name and a sequence of fields
      auto [obj_name, obj_flds] =
        unpackSingleEntryMap(obj_entry, JMG_OBJS_ENTRY());
      enforceType(obj_flds, NodeType::Sequence, "fields of ", JMG_OBJS_ENTRY());
      for (auto [fld_idx, fld_entry] : vws::enumerate(obj_flds)) {
        spec.processObjFld(obj_name, makeObjNode(fld_entry, "field [", fld_idx,
                                                 "] of ", JMG_OBJS_ENTRY()));
      }
    }
#undef JMG_OBJS_ENTRY
  }
}

#undef JMG_FWD_DESC

#undef JMG_STR_DESC

} // namespace yaml
} // namespace jmgc
