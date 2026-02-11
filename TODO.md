
# Quick Lists of Short to Medium Term TODO Items

## Misc Standardization

* Use `JMG_TAG_TYPE` to declare all tag types
* Update standard `JMG_OBJECT_CONCEPT` to include a requirement to be
  derived from `jmg::ObjectDefT`?
* Investigate sketchy results for testing argument type generation for
  safe type fields in test_field.cpp
  * Argument type does not match the safe type exactly, although
    `type_name_for<>()` produces the same string in both cases.
  * Generated argument type is a non-`const` ref instead of a `const`
    ref
* Investigate sketchy results for test_native, there are several TODO
  items which indicate that return types are not yet being generated
  correctly.
* Investigate handling of get() and try_get() in quickfix spec and
  modify them so that have standard behavior (if appropriate,
  otherwise document the differences)
* Review the handling of optional fields in _yaml_ encoding and
  consider whether it correctly fits the standard patterns.
  * If not, what modifications should be made to either the _yaml_
    encoding or to the standard patterns?

## Naming

* Concepts and type metafunctions shall be named with a trailing `T`.
  * Should concepts use a different scheme, e.g. trailing `C`?
* Type aliases and template type parameter names **shall not** be
  named with a trailing `T`.

List of incorrectly named type aliases, template type parameters,
concepts and type metafunctions:

* ~~`Decay` (**meta.h**) -> `DecayT`~~

## Layout

* Move each encoding type to a separate bazel `cc_library` target.
* Move header files for encodings to the main include directory or put
  them together in a directory named **encodings**?

## All Fields

* Unify handling of JMG objects
  * JMG objects are proxy objects and thus have different return types
    than regular non-primitive, non-viewable types
    * see protobuf implementation
  * Maybe move the type metafunctions that determine return type from
    field.h to object.h?
* unify handling of ArrayField and ArrayProxy
* strengthen handling of safe types, especially those that wrap
  strings and arrays
* systematic handling of types that can be viewed by either
  `std::string_view` or `std::span`
* unify naming and structure across all object types

## All Objects

* Support the declaration of objects using `meta::list` of `FieldDef`
  in addition to parameter pack of `Field`
* ~~Standardized naming~~
  * ~~All _field_ and _object_ types shall be declared in a namespace
    specific to the encoding, e.g. `jmg::yaml::Object`~~
  * ~~There shall be a `FieldTag` and an `ObjectTag` declared in a
    `detail` namespace of the main encoding namespace,
    e.g. `jmg::yaml::ObjectTag`~~
  * ~~_field_ type requirements:~~
    * ~~Shall be named `Field`, e.g. `jmg::yaml::Field`~~
    * ~~Shall be derived from `detail::FieldTag` with `public`
      visibility~~
    * ~~Shall have a related `FieldT` concept in the main encoding
      namespace that relies of derivation from `FieldTag`.~~
  * ~~_object_ type requirements:~~
    * ~~Shall be named `Object`, e.g. `jmg::yaml::Object`~~
    * ~~Shall be derived from `detail::ObjectTag` with `public`
      visibility~~
    * ~~Shall have a related `ObjectT` concept in the main encoding
      namespace that relies of derivation from `ObjectTag`.~~
* ~~correctly handle get/try_get for const and non-const objects~~
* proper handling of object mutation
  * stage 1 - all mutation through `jmg::set()`
  * stage 2 - investigate alternatives for collections
    * alternative 1 - introduce e.g. `jmg::add()` et.al. similar to
      handling of `repeated` fields in protobuf.
      * Note that some work along these lines has already been done
        with the `clear()` member function in the _native_ encoding
        object.
    * alternative 2 - develop proxy objects where necessary to support
      emulation of the style of interaction used in the standard
      library
  * stage 3 - investigage systematic replacement of `jmg::set()` (and
    any other intermediate constructs) with allowing
    `jmg::get()`/`jmg::try_get()` to return non-`const`
    objects/proxies to allow JMG objects to be treated consistently
    with e.g. `std::tuple` and `std::variant`

## Protobuf objects

* protobuf 'bytes' type should work with BufferView?
* Support enums
* ~~Support single class fields~~
* Support repeated string fields
* Support repeated class fields
* Support `oneof` fields using `jmg::Union`
* Modify jmgc to generate .proto files using JMG YAML spec as the
  source of ground truth

## YAML objects

* Correctly support StringField instead of allowing FieldDef<string...>

# Ongoing/interrupted project notes

## jmgc support for protobuf

Currently on branch `features/more-encodings`, evolution of the code
indicates a need for substantial rework of metaprogramming mechanisms
for determining the return types of `jmg::get()` and `jmg::try_get()`.

## support for flatbuffers

This is extremely hairy since flatbuffers has no reflection interface
like protobuf does. Flatbuffers `FieldDef` declarations may need to
include retrieval functions via lambda or some other mechanism. Seems
like a place where macros would be beneficial and there would be tight
coupling between the _field_ and _object_ definitions.

Another alternative would be to break open the serialization encoding
and tie the _object_ implementation to it, but this seems excessive...
