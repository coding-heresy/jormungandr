#pragma once

#include <tuple>

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// helper functions for accessing key and value fields of a map entry
////////////////////////////////////////////////////////////////////////////////
const auto& key_of(const auto& rec) {
  return std::get<0>(rec);
}

const auto& value_of(const auto& rec) {
  return std::get<1>(rec);
}

auto& value_of(auto& rec) {
  return std::get<1>(rec);
}
} // namespace jmg
