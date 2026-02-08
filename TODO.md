
# Quick Lists of Short to Medium Term TODO Items

## All Fields

* unify handling of ArrayField and ArrayProxy
* strengthen handling of safe types, especially those that wrap
  strings and arrays
* systematic handling of types that can be viewed by either
  `std::string_view` or `std::span`
* unify naming and structure across all object types

## All Objects

* ~~correctly handle get/try_get for const and non-const objects~~
* proper handling of object mutation
  * stage 1 - all mutation through `jmg::set()`
  * stage 2 - investigate alternatives for collections
    * alternative 1 - introduce e.g. `jmg::add()` et.al. similar to
      handling of `repeated` fields in protobuf
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
* Support single class fields
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
