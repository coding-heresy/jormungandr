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

#include <concepts>
#include <string>
#include <string_view>

#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/repeated_ptr_field.h>

#include "jmg/field.h"
#include "jmg/meta.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"
#include "jmg/util.h"

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
 * concept for protobuf messages
 */
template<typename T>
concept ProtoMsgT =
  std::derived_from<Decay<T>, google::protobuf::Message>
  || std::derived_from<Decay<T>, google::protobuf::MessageLite>;

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
concept HeavyProtoMsgT = std::derived_from<Decay<T>, google::protobuf::Message>;

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
/**
 * tag type used to indicate that a type is a protobuf field
 * definition
 */
struct ProtoFieldTag {};

/**
 * tag type used to indicate that a JMG object wraps a protobuf
 * message
 */
struct ObjectTag {};

/**
 * concept for a supported underlying field type that is not either a
 * string or an array
 */
template<typename T>
concept NonSpecializedTypeT =
  ScalarTypeT<T> || std::derived_from<Decay<T>, detail::ObjectTag>;

} // namespace detail

/**
 * declaration of a data field for a protobuf
 * @tparam T type of data associated with the field
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 * @tparam kId Protobuf field ID
 */
#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
template<detail::NonSpecializedTypeT T,
#else
template<ScalarTypeT T,
#endif
         StrLiteral kName,
         TypeFlagT IsRequired,
         int kId>
struct FieldDef : public jmg::FieldDef<T, kName, IsRequired>,
                  detail::ProtoFieldTag {
  static constexpr auto id = kId;
  static_assert(kId > 0, "protobuf field identifier must be non-negative");
};

/**
 * special handling for fields containing strings
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 * @tparam kId Protobuf field ID
 */
template<StrLiteral kName, TypeFlagT IsRequired, int kId>
struct StringField : public jmg::StringField<kName, IsRequired>,
                     detail::ProtoFieldTag {
  static constexpr auto id = kId;
  static_assert(kId > 0, "protobuf field identifier must be non-negative");
};

/**
 * special handling for repeated fields
 * @tparam T type of repeated field
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 * @tparam kId Protobuf field ID
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired, int kId>
// TODO(bd) add support for arrays of sub-objects
  requires(isMemberOfList<T, ScalarTypes>() || SameAsDecayedT<std::string, T>)
struct ArrayField : public jmg::ArrayField<T, kName, IsRequired>,
                    detail::ProtoFieldTag {
  static constexpr auto id = kId;
  static_assert(kId > 0, "protobuf field identifier must be non-negative");
};

namespace detail
{

/**
 * field definition must have an 'id' static data member that is an
 * integer constant.
 */
template<typename T>
concept HasProtoFieldId = requires { std::same_as<decltype(T::id), int>; };

} // namespace detail

/**
 * Concept for field definition
 */
template<typename T>
concept ProtoFieldT =
  std::derived_from<T, detail::ProtoFieldTag> && jmg::detail::HasFieldName<T>
  && jmg::detail::HasFieldType<T> && jmg::detail::HasRequiredSpec<T>
  && detail::HasProtoFieldId<T>;

/**
 * wrapper type for protobuf object
 *
 * TODO(bd) use metaprogramming to enforce that there can be no
 * duplicate field IDs
 */
template<HeavyProtoMsgT Msg, ProtoFieldT... Fields>
class Object : public jmg::ObjectDef<Fields...>, public detail::ObjectTag {
  using Descriptor = google::protobuf::Descriptor;
  using Reflection = google::protobuf::Reflection;
  using FieldDescriptor = google::protobuf::FieldDescriptor;

public:
  Object() = delete;
  explicit Object(const Msg& msg)
    : msg_(msg), pd_(*(msg.GetDescriptor())), mr_(*(msg.GetReflection())) {}

  /**
   * delegate for jmg::get()
   */
  template<RequiredFieldT Fld>
  ReturnTypeForFieldT<Fld> get() const {
    using Type = typename Fld::type;
    static constexpr auto field_name = std::string_view(Fld::name);
    const auto& field_des = getFieldDescriptor<Fld::id>(field_name);
    enforcePresence(field_des, field_name);
    return getByType<Fld>(field_des);
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalFieldT Fld>
  ReturnTypeForFieldT<Fld> try_get() const {
    using Type = typename Fld::type;
    using Rslt = ReturnTypeForFieldT<Fld>;
    static constexpr auto field_name = std::string_view(Fld::name);
    const auto& field_des = getFieldDescriptor<Fld::id>(field_name);
    if (!isPresent(field_des)) { return std::nullopt; }
    return Rslt(getByType<Fld>(field_des));
  }

private:
  /**
   * retrieve the descriptor for a specific field of the protobuf
   */
  template<int kFldId>
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

  template<FieldDefT Fld>
  auto getByType(const FieldDescriptor& field_des) const {
    using Type = typename Fld::type;
#if !defined(NDEBUG)
    enforcePresence(field_des, Fld::name);
#endif
    // case analysis of possible field types, embedding type names in
    // function names is very annoying...
    if constexpr (jmg::SameAsDecayedT<bool, Type>) {
      return mr_.GetBool(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<int32_t, Type>) {
      return mr_.GetInt32(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<uint32_t, Type>) {
      return mr_.GetUInt32(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<int64_t, Type>) {
      return mr_.GetInt64(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<uint64_t, Type>) {
      return mr_.GetUInt64(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<float, Type>) {
      return mr_.GetFloat(msg_, &field_des);
    }
    else if constexpr (jmg::SameAsDecayedT<double, Type>) {
      return mr_.GetDouble(msg_, &field_des);
    }
#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
    else if constexpr (jmg::ObjectT<Decay<Type>>) {
      const auto& sub_msg = mr_.GetMessage(msg_, &field_des);
      return dynamic_cast<ReturnTypeForFieldT<Fld>>(sub_msg);
    }
#endif
    else if constexpr (StringFieldT<Fld>) {
      // NOTE: scratch space should never be required because
      // getByType should never be called when the field is not
      // present in the object
      std::string scratch;
      return std::string_view(mr_.GetStringReference(msg_, &field_des,
                                                     &scratch));
    }
    else if constexpr (ArrayFieldT<Fld>) {
      using ValueType = Decay<typename Fld::view_type::value_type>;
      if constexpr (ScalarTypeT<ValueType>) {
        const auto& raw_field =
          mr_.GetRepeatedField<ValueType>(msg_, &field_des);
        return std::span<const ValueType>(raw_field.data(), raw_field.size());
      }
      else {
        // TODO(bd) use the RepeatedPtrField class and a proxy to get
        // data for a repeated non-primitive field
        static_assert(
          false,
          "retrieving data for repeated non-primitive fields is not supported");
      }
    }

    // TODO(bd) handle repeated and complex types

    else { JMG_NOT_EXHAUSTIVE(Fld); }
  }

  const Msg& msg_;
  const Descriptor& pd_;
  const Reflection& mr_;
};

} // namespace jmg::protobuf
