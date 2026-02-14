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
#pragma once

#include <iterator>
#include <string>
#include <string_view>

#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/repeated_ptr_field.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

#include "jmg/conversion.h"
#include "jmg/field.h"
#include "jmg/meta.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"
#include "jmg/types.h"
#include "jmg/util.h"

namespace
{
using CppType = google::protobuf::FieldDescriptor::CppType;
} // namespace

namespace jmg::protobuf
{

/**
 * list of scalar types supported by protobufs
 */
using ScalarTypes =
  meta::list<bool, uint32_t, int32_t, uint64_t, int64_t, float, double>;

/**
 * concept for a scalar type
 */
template<typename T>
concept ScalarTypeT = isMemberOfList<T, ScalarTypes>();

/**
 * concept for protobuf messages (NOT for JMG objects that wrap
 * protobuf messages)
 */
template<typename T>
concept ProtoMsgT =
  std::derived_from<DecayT<T>, google::protobuf::Message>
  || std::derived_from<DecayT<T>, google::protobuf::MessageLite>;

/**
 * helper function template that throws exception if a protobuf is
 * missing required fields
 */
template<ProtoMsgT Msg>
static void verify(const Msg& proto, const std::string_view op) {
  if (!proto.IsInitialized()) {
    const auto err_msg =
      str_cat("unable to ", op,
              " protobuf object due to missing required fields [",
              proto.InitializationErrorString(), "]");
    throw std::runtime_error(err_msg);
  }
}

/**
 * helper function template that serializes a protobuf
 */
template<ProtoMsgT Msg>
static std::string serialize(const Msg& proto) {
  using namespace std::string_view_literals;
  verify(proto, "serialize"sv);
  std::string buf;
  if (!proto.SerializeToString(&buf)) {
    const auto err_msg =
      str_cat("unable to serialize protobuf object due to "
              "unknown reason other than missing required fields");
    throw std::runtime_error(err_msg);
  }
  return buf;
}

/**
 * helper function template that deserializes a protobuf
 */
template<ProtoMsgT Msg>
static Msg deserialize(const std::string_view buf) {
  Msg rslt;
  if (!rslt.ParseFromString(buf)) {
    // try partial parsing to check for missing fields
    Msg tmp;
    tmp.ParsePartialFromString(buf);
    verify(tmp, "deserialize");

    // if this point was reached the failure reason was not missing
    // fields
    const auto err_msg =
      str_cat("unable to deserialize protobuf object due to "
              "unknown reason other than missing required fields");
    throw std::runtime_error(err_msg);
  }
  return rslt;
}

/**
 * concept for non-lite protobuf message that supports reflection and
 * can thus be used for jmg::protobuf::Object
 */
template<typename T>
concept HeavyProtoMsgT =
  std::derived_from<DecayT<T>, google::protobuf::Message>;

/**
 * safe type for field ID
 *
 * TODO(bd) remove?
 */
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using ProtoFieldId = SafeType<int, SafeIdType>;
#else
JMG_NEW_SAFE_TYPE(ProtoFieldId, int, SafeIdType);
#endif

namespace detail
{
JMG_TAG_TYPE(Field);
JMG_TAG_TYPE(Object);
} // namespace detail

JMG_OBJECT_CONCEPT();

namespace detail
{

/**
 * concept for a supported underlying field type that is not either a
 * string or an array
 *
 * NOTE: the only non-primitive, non-viewable types allowed are other
 * protobuf::Object types (i.e. types generated from protobuf
 * messages) and the JMG standard TimePoint type
 */
template<typename T>
concept NonSpecializedTypeT =
  ScalarTypeT<T>
  || protobuf::ObjectT<T>
  // NOTE: TimePoint is a special case because
  // google::protobuf::Timestamp is the standard representation for it
  || SameAsDecayedT<TimePoint, T>;

} // namespace detail

/**
 * declaration of a data field for a protobuf
 * @tparam T type of data associated with the field
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 * @tparam kId Protobuf field ID
 */
template<detail::NonSpecializedTypeT T,
         StrLiteral kName,
         TypeFlagT IsRequired,
         uint32_t kFieldId>
struct Field : public jmg::FieldDef<T, kName, IsRequired>,
               public detail::FieldTag {
  static constexpr auto kFldId = kFieldId;
};

/**
 * special handling for fields containing strings
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 * @tparam kId Protobuf field ID
 */
template<StrLiteral kName, TypeFlagT IsRequired, uint32_t kFieldId>
struct StringField : public jmg::StringField<kName, IsRequired>,
                     detail::FieldTag {
  static constexpr auto kFldId = kFieldId;
};

/**
 * special handling for repeated fields
 * @tparam T type of repeated field
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 * @tparam kId Protobuf field ID
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired, uint32_t kFieldId>
// TODO(bd) add support for arrays of sub-objects
  requires(isMemberOfList<T, ScalarTypes>() || SameAsDecayedT<std::string, T>)
struct ArrayField : public jmg::ArrayField<T, kName, IsRequired>,
                    detail::FieldTag {
  static constexpr auto kFldId = kFieldId;
};

namespace detail
{

/**
 * field definition must have an 'id' static data member that is an
 * integer constant.
 */
template<typename T>
concept HasProtoFieldId = requires { std::same_as<decltype(T::kFldId), int>; };

} // namespace detail

/**
 * Concept for field definition
 */
template<typename T>
concept FieldT = std::derived_from<T, detail::FieldTag> && jmg::FieldDefT<T>
                 && detail::HasProtoFieldId<T>;

#if !defined(NDEBUG)
#define JMG_ENFORCE_TYPE_MATCH(fld, expected_type)                 \
  JMG_ENFORCE(expected_type == fld.cpp_type(), "field has type [", \
              fld.cpp_type(), "] but expected type [", expected_type, "]")
#else
#define JMG_ENFORCE_TYPE_MATCH(fld, expected_type)
#endif

/**
 * wrapper type for protobuf object
 *
 * TODO(bd) use metaprogramming to enforce that there can be no
 * duplicate field IDs
 */
template<HeavyProtoMsgT Msg, protobuf::FieldT... Fields>
class Object : public jmg::ObjectDef<Fields...>, public detail::ObjectTag {
  using Descriptor = google::protobuf::Descriptor;
  using Reflection = google::protobuf::Reflection;
  using FieldDescriptor = google::protobuf::FieldDescriptor;
  using Message = google::protobuf::Message;

public:
  using MsgType = Msg;

  Object() = delete;
  explicit Object(const Msg& msg)
    : msg_(msg)
    , pd_(*(msg.GetDescriptor()))
    , mr_(*(msg.GetReflection()))
    , msg_ptr_(nullptr) {}
  explicit Object(Msg& msg)
    : msg_(msg)
    , pd_(*(msg.GetDescriptor()))
    , mr_(*(msg.GetReflection()))
    , msg_ptr_(&msg) {}

  /**
   * delegate for jmg::get()
   */
  template<RequiredFieldT Fld>
  decltype(auto) get() const {
    using Type = typename Fld::type;
    static constexpr auto field_name = std::string_view(Fld::name);
    const auto& field_des = getFieldDescriptor<Fld::kFldId>(field_name);
    enforcePresence(field_des, field_name);
    return getByType<Fld>(field_des);
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalFieldT Fld>
  decltype(auto) try_get() const {
    using Type = typename Fld::type;
    static constexpr auto field_name = std::string_view(Fld::name);
    const auto& field_des = getFieldDescriptor<Fld::kFldId>(field_name);
    if constexpr (ObjectT<Type> || SameAsDecayedT<TimePoint, Type>) {
      using Rslt = std::optional<Type>;
      if (!isPresent(field_des)) { return Rslt(std::nullopt); }
      return Rslt(getByType<Fld>(field_des));
    }
    else {
      using Rslt = ReturnTypeForFieldT<Fld>;
      if (!isPresent(field_des)) { return Rslt(std::nullopt); }
      return Rslt(getByType<Fld>(field_des));
    }
  }

  /**
   * delegate for jmg::set() for non-viewable fields
   */
  template<NonViewableFieldT Fld>
  void set(ArgTypeForFieldT<Fld> arg) {
    using ArgType = ArgTypeForFieldT<Fld>;
    using Type = typename Fld::type;
    static constexpr auto field_name = std::string_view(Fld::name);
    const auto& field_des = getFieldDescriptor<Fld::kFldId>(field_name);
    if constexpr (ObjectT<Type>) {
      // TODO(bd) handle message types
      JMG_ENFORCE(false, "not yet supported");
    }
    else { setByType<Fld>(field_des, std::forward<ArgType>(arg)); }
  }

  /**
   * delegate for jmg::set() for viewable fields
   */
  template<ViewableFieldT Fld>
  void set(ArgTypeForFieldT<Fld> arg) {
    static constexpr auto field_name = std::string_view(Fld::name);
    const auto& field_des = getFieldDescriptor<Fld::kFldId>(field_name);
    if constexpr (StringFieldT<Fld>) {
      // TODO(bd) current version of reflection interface only
      // supports arguments of type std::string
      setByType<Fld>(field_des, std::string(arg));
    }
    // TODO(bd) handle array types
    else { JMG_NOT_EXHAUSTIVE(Fld); }
  }

private:
  /**
   * class template for an array proxy associated which wraps a field of
   * repeated string objects
   */
  template<protobuf::HeavyProtoMsgT ProtoMsg, int32_t kFldId>
  class RptStrArrayProxy {
    using Descriptor = google::protobuf::Descriptor;
    using Reflection = google::protobuf::Reflection;
    using FieldDescriptor = google::protobuf::FieldDescriptor;

  public:
    RptStrArrayProxy() = delete;
    ~RptStrArrayProxy() = default;
    RptStrArrayProxy(const ProtoMsg& msg)
      : msg_(&msg)
      , pd_(msg.GetDescriptor())
      , mr_(msg.GetReflection())
      , fd_(pd_->FindFieldByNumber(kFldId)) {}

    /**
     * class template for an iterator proxy that allows a repeated list of
     * protobuf strings to be viewed as a range of std::string_view
     */
    class RptStrItr {
      using Descriptor = google::protobuf::Descriptor;
      using Reflection = google::protobuf::Reflection;
      using FieldDescriptor = google::protobuf::FieldDescriptor;

    public:
      // aliases required to allow the iterator to be used in
      // std::ranges algorithms
      using value_type = std::string_view;
      using difference_type = int;
      using iterator_category = std::input_iterator_tag;
      using iterator_concept = std::forward_iterator_tag;
      using reference = std::string_view;

      RptStrItr() = default;
      ~RptStrItr() = default;
      JMG_DEFAULT_COPYABLE(RptStrItr);
      JMG_DEFAULT_MOVEABLE(RptStrItr);

      // TODO(bd) remove?
      explicit RptStrItr(const ProtoMsg& msg)
        : msg_(&msg)
        , pd_(msg.GetDescriptor())
        , mr_(msg.GetReflection())
        , fd_(pd_->FindFieldByNumber(kFldId))
        , idx_(0)
        , sz_(mr_->FieldSize(msg, fd_)) {
#if !defined(NDEBUG)
        JMG_ENFORCE(fd_, "unable to get field descriptor for field [", kFldId,
                    "]");
        JMG_ENFORCE(fd_->is_repeated(), "field [", kFldId, "] is not repeated");
        JMG_ENFORCE(fd_->cpp_type() == FieldDescriptor::CPPTYPE_STRING,
                    "field [", kFldId, "] does not contain strings");
#endif
        if (0 == sz_) { clear(); }
      }

      std::string_view operator*() const {
        const auto& ref =
          mr_->GetRepeatedStringReference(*msg_, fd_, idx_, &scratch_);
        return std::string_view(ref.data(), ref.size());
      }

      RptStrItr& operator++() {
        if (-1 == idx_) { return *this; }
        ++idx_;
        if (static_cast<size_t>(idx_) == sz_) { clear(); }
        return *this;
      }

      RptStrItr operator++(int) {
        if (-1 == idx_) { return *this; }
        auto rslt = *this;
        ++idx_;
        if (static_cast<size_t>(idx_) == sz_) { clear(); }
        return rslt;
      }

      // TODO(bd) use friend operators
      bool operator==(const RptStrItr& that) const {
        return (this->msg_ == that.msg_) && (this->pd_ == that.pd_)
               && (this->mr_ == that.mr_) && (this->fd_ == that.fd_)
               && (this->idx_ == that.idx_);
      }

      bool operator!=(const RptStrItr& that) const {
        return (this->msg_ != that.msg_) || (this->pd_ != that.pd_)
               || (this->mr_ != that.mr_) || (this->fd_ != that.fd_)
               || (this->idx_ != that.idx_);
      }

    private:
      friend class RptStrArrayProxy<ProtoMsg, kFldId>;

      RptStrItr(const ProtoMsg* msg,
                const Descriptor* pd,
                const Reflection* mr,
                const FieldDescriptor* fd)
        : msg_(msg)
        , pd_(pd)
        , mr_(mr)
        , fd_(fd)
        , idx_(0)
        , sz_(mr->FieldSize(*msg, fd)) {}

      void clear() {
        msg_ = nullptr;
        pd_ = nullptr;
        mr_ = nullptr;
        fd_ = nullptr;
        idx_ = -1;
      }

      const ProtoMsg* msg_ = nullptr;
      const Descriptor* pd_ = nullptr;
      const Reflection* mr_ = nullptr;
      const FieldDescriptor* fd_ = nullptr;
      int idx_ = -1;
      size_t sz_;
      mutable std::string scratch_;
    };

    auto size() const { return mr_->FieldSize(*msg_, fd_); }
    auto empty() const { return 0 == size(); }

    auto begin() const { return RptStrItr(msg_, pd_, mr_, fd_); };
    auto end() const { return RptStrItr(); };

    auto cbegin() const { return RptStrItr(msg_, pd_, mr_, fd_); };
    auto cend() const { return RptStrItr(); };

  private:
    const ProtoMsg* msg_ = nullptr;
    const Descriptor* pd_ = nullptr;
    const Reflection* mr_ = nullptr;
    const FieldDescriptor* fd_ = nullptr;
  };

  /**
   * retrieve the descriptor for a specific field of the protobuf
   */
  template<uint32_t kFldId>
  const FieldDescriptor& getFieldDescriptor(
    const std::string_view field_name) const {
    const auto* ptr = pd_.FindFieldByNumber(kFldId);
    JMG_ENFORCE(ptr, "unable to get protobuf reflection descriptor for field [",
                field_name, "] using ID [", kFldId, "]");
    return *ptr;
  }

  /**
   * check if a field is present in the message
   */
  bool isPresent(const FieldDescriptor& field) const {
    if (!field.is_repeated()) { return mr_.HasField(msg_, &field); }
    // repeated fields always have presence, by construction
    return true;
  }

  /**
   * throw exception if a required field has no value
   */
  void enforcePresence(const FieldDescriptor& field,
                       const std::string_view name) const {
    if (!field.is_repeated()) {
      JMG_ENFORCE(mr_.HasField(msg_, &field), "required field [", name,
                  "] has no value set");
    }
  }

  /**
   * function template that uses metaprogramming analysis of the field
   * type to call the correct reflection member function for the type
   * being retrieved
   */
  template<protobuf::FieldT Fld>
  decltype(auto) getByType(const FieldDescriptor& field_des) const {
    using namespace google::protobuf;
    using Type = typename Fld::type;
#if !defined(NDEBUG)
    enforcePresence(field_des, Fld::name);
#endif
    // case analysis of possible field types, embedding type names in
    // function names is very annoying...
    if constexpr (jmg::SameAsDecayedT<bool, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_BOOL);
      return mr_.GetBool(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<int32_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_INT32);
      return mr_.GetInt32(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<uint32_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_UINT32);
      return mr_.GetUInt32(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<int64_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_INT64);
      return mr_.GetInt64(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<uint64_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_UINT64);
      return mr_.GetUInt64(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<float, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_FLOAT);
      return mr_.GetFloat(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<double, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_DOUBLE);
      return mr_.GetDouble(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<jmg::TimePoint, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_MESSAGE);
      const auto& generic_ts_msg = mr_.GetMessage(msg_, &field_des);
      const auto& ts_msg =
        *(DynamicCastToGenerated<Timestamp>(&generic_ts_msg));
      TimePoint tp = from(ts_msg);
      return tp;
    }
    else if constexpr (jmg::ObjectT<DecayT<Type>>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_MESSAGE);
      const auto& generic_sub_msg = mr_.GetMessage(msg_, &field_des);
      using Wrapper = typename Fld::type;
      using Wrapped = typename Wrapper::MsgType;
      const auto& sub_msg = DynamicCastToGenerated<Wrapped>(generic_sub_msg);
      return Wrapper(sub_msg);
    }
    else if constexpr (StringFieldT<Fld>) {
      // NOTE: scratch space should never be required because
      // getByType should never be called when the field is not
      // present in the object
      std::string scratch;
      return std::string_view(mr_.GetStringReference(msg_, &field_des,
                                                     &scratch));
    }
    else if constexpr (ArrayFieldT<Fld>) {
      using ValueType = DecayT<typename Fld::view_type::value_type>;
      if constexpr (ScalarTypeT<ValueType>) {
        const auto& raw_field =
          mr_.GetRepeatedField<ValueType>(msg_, &field_des);
        return std::span<const ValueType>(raw_field.data(), raw_field.size());
      }
      else if constexpr (SameAsDecayedT<std::string, ValueType>) {
        return RptStrArrayProxy<Msg, Fld::kFldId>(msg_);
      }
      else {
        // TODO(bd) use the RepeatedPtrField class and a proxy to get
        // data for a repeated non-primitive field
        static_assert(
          false,
          "retrieving data for repeated non-primitive fields is not supported");
      }
    }

    // TODO(bd) handle repeated types

    else { JMG_NOT_EXHAUSTIVE(Type); }
  }

  /**
   * function template that uses metaprogramming analysis of the field
   * type to call the correct reflection member function for the type
   * being set
   */
  template<protobuf::FieldT Fld>
  void setByType(const FieldDescriptor& field_des, typename Fld::type&& val) {
    using namespace google::protobuf;
    using Type = typename Fld::type;
    // case analysis of possible field types, embedding type names in
    // function names is very annoying...
    if constexpr (jmg::SameAsDecayedT<bool, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_BOOL);
      mr_.SetBool(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<int32_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_INT32);
      mr_.SetInt32(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<uint32_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_UINT32);
      mr_.SetUInt32(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<int64_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_INT64);
      mr_.SetInt64(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<uint64_t, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_UINT64);
      mr_.SetUInt64(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<float, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_FLOAT);
      mr_.SetFloat(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<double, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_DOUBLE);
      mr_.SetDouble(msg_ptr_, &field_des, val);
    }
    else if constexpr (jmg::SameAsDecayedT<jmg::TimePoint, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_MESSAGE);
      Message* generic_ts_msg = mr_.MutableMessage(msg_ptr_, &field_des);
      auto& ts_msg = *(DynamicCastToGenerated<Timestamp>(generic_ts_msg));
      ts_msg = from(val);
    }
    else if constexpr (jmg::SameAsDecayedT<std::string, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_STRING);
      mr_.SetString(msg_ptr_, &field_des, std::move(val));
    }

    // TODO(bd) handle string types

    // TODO(bd) handle repeated and non-primitive types

    else { JMG_NOT_EXHAUSTIVE(Type); }
  }

  /**
   * TODO(bd) figure out how to make the other override of setByType
   * correctly perform perfect forwarding so it works on const ref
   * value arguments
   */
  template<protobuf::FieldT Fld>
  void setByType(const FieldDescriptor& field_des,
                 const typename Fld::type& val) {
    using namespace google::protobuf;
    using Type = typename Fld::type;
    if constexpr (jmg::SameAsDecayedT<jmg::TimePoint, Type>) {
      JMG_ENFORCE_TYPE_MATCH(field_des, CppType::CPPTYPE_MESSAGE);
      Message* generic_ts_msg = mr_.MutableMessage(msg_ptr_, &field_des);
      auto& ts_msg = *(DynamicCastToGenerated<Timestamp>(generic_ts_msg));
      ts_msg = from(val);
    }

    // TODO(bd) add other cases here if perfect forwarding can't be
    // fixed for the other override

    else { JMG_NOT_EXHAUSTIVE(Type); }
  }

  const Msg& msg_;
  const Descriptor& pd_;
  const Reflection& mr_;
  // NOTE: non-const pointer is only set if the object was constructed
  // from a non-const generated protobuf object
  Message* msg_ptr_;
};

#undef JMG_ENFORCE_TYPE_MATCH

} // namespace jmg::protobuf
