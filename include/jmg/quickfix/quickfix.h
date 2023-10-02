#pragma once

#include <quickfix/Message.h>

#include "jmg/conversion.h"
#include "jmg/field.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"

namespace jmg
{

namespace quickfix
{

template<typename... Fields>
class Object : public ObjectDef<Fields...>
{
public:
  using adapted_type = FIX::Message;

  Object() = delete;
  explicit Object(const adapted_type& adapted) : adapted_(&adapted) {}


  /**
   * delegate for jmg::get()
   */
  template<RequiredField FldT>
  typename FldT::type get() const {
    return getImpl<FldT>();
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField FldT>
  std::optional<typename FldT::type> try_get() const {
    if (!adapted_->isSetField(FldT::kFixTag)) {
      return std::nullopt;
    }
    return getImpl<FldT>();
  }

private:
  /**
   * implementation for get/try_get
   */
  template<typename FldT>
  typename FldT::type getImpl() const
    requires ConversionTgtT<typename FldT::type>
  {
    const std::string& fieldRef = adapted_->getField(FldT::kFixTag);
    return static_cast<typename FldT::type>(from_string(fieldRef));
  }
  const adapted_type* adapted_;
};

} // namespace quickfix
} // namespace jmg
