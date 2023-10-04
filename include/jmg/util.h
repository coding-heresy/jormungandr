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

template<typename Fcn>
struct AutoCleanup
{
  AutoCleanup(Fcn&& fcn) : fcn_(std::move(fcn)) {}
  ~AutoCleanup() {
    if (isActive_) { fcn_(); }
  }
  void cancel() { isActive_ = false; }

private:
  bool isActive_ = true;
  Fcn fcn_;
};

} // namespace jmg
