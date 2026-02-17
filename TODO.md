
# Quick Lists of Short to Medium Term TODO Items

## Misc other/minor

* Modify `find_required` to support `string_view` object for key if the
  dictionary key type is `string`
* Create `identifier` and `arithmetic` traits for safe types that
  derive from or are aliased to the appropriate traits in the `st::`
  namespace, if possible
* Check if array proxy currently includes an iterator type that wraps
  an index into an array-like structure that the iterator can hold a
  reference to
  * Also check if it makes sense to create a mixin type that enables
    custom iterators to support `std::views` automatically
* Constrain the type parameters allowable in `yaml::Object`
  declarations (see TODO comment in situ)
* `value_of` and `key_of` should automatically look through iterators
* Should scoped enums be prohibited from being wrapped in safe types?
* Add standardized logging?
* Should safe types in general be limited in terms of what types they
  can wrap?
  * Consider the following idea: _utility_ types. Many/most _class_
    types are very specialized and wrapping them in safe types is not
    valuable, but there are some _class_ types (in addition to
    _primitive_ types) that are of more general utility, very akin to
    _primitive_ types. These _utility_ types would include
    _string_/_string-like_ types, time points and time
    durations. There is possibly a linkage between _utiity_ types and
    types handled by `from()` (or maybe just having such a set of
    types is giving me tunnel vision and preventing me from seeing
    other types that fit this description).
* Add more `cc_build_error_test` cases
  * Test cases from the bottom of test_cmdline.cpp

## jmgc

* Generation of encoding wrappers is broken because all fields are
  generated before any objects
  * The correct approach is to explicitly determine dependencies
    between objects using a DAG and then walk the DAG as appropriate,
    with each object emitting any required fields that have not yet
    been emitted.
* JMG IDL implementations in jmgc should temporarily explicitly not
  allow `union` types at this point
* Support `import` section for JMG IDL
  * Field definitions exist but `processYamlFile()` will probably need
    to be modified to allow it to be recursively called in a mode that
    should mostly populate the symbol table (such as it is)
* Fix parsing of `values` field for `enum` types so that the name of
  the enumeration doesn't require a separate `name` field, similar to
  how lists are handled in `types` and `objects`
* Presence of `protobuf_imports` should trigger an appropriate
  `#include` in proto encoding headers

* Test cases
  * `enum` type whose `values` field is empty
  * `protobuf_imports` field of `package` that contains no files

## Naming

* Concepts and type metafunctions shall be named with a trailing `T`.
  * Should concepts use a different scheme, e.g. trailing `C`?
* Type aliases and template type parameter names **shall not** be
  named with a trailing `T`.
* Should `Field` be collapsed to `Fld` everywhere?
* Should `Return` be collapsed to `Rslt` everywhere?

List of incorrectly named type aliases, template type parameters,
concepts and type metafunctions:

* ~~`Decay` (**meta.h**) -> `DecayT`~~

## Layout

* Move each encoding type to a separate bazel `cc_library` target.
* Move header files for encodings to the main include directory or put
  them together in a directory named **encodings**?

## All Fields

* Make all field definitions consistent
* Decide whether `ArrayField` should explicitly specify `std::vector`
  as the container type
  * This is actually depends on the implementation of
    _vector_/_array_/_repeated_ fields in the underlying encoding.
* Unify handling of JMG objects
  * JMG objects are proxy objects and thus have different return types
    than regular non-primitive, non-viewable types
    * see protobuf implementation
  * Maybe move the type metafunctions that determine return type from
    field.h to object.h?
  * In general, it seems like the only class types other than viewable
    types, safe types and `jmg:TimePoint` should always satisfy
    `jmg::ObjectT`; need to look for counter-examples
* unify handling of ArrayField and ArrayProxy
* strengthen handling of safe types, especially those that wrap
  strings and arrays
* systematic handling of types that can be viewed by either
  `std::string_view` or `std::span`
* unify naming and structure across all object types

## All Objects

* Make all object definitions consistent
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
    * stage 1a - ensure that `jmg::set()` mutation supports move
      semantics where possible/appropriate
  * stage 2 - investigate alternatives for collections
    * alternative 1 - introduce e.g. `jmg::add()` et.al. similar to
      handling of `repeated` fields in protobuf.
      * Note that some work along these lines has already been done
        with the `clear()` member function in the _native_ encoding
        object.
    * alternative 2 - develop proxy objects where necessary to support
      emulation of the style of interaction used in the standard
      library
  * stage 3 - investigate systematic replacement of `jmg::set()` (and
    any other intermediate constructs) with allowing
    `jmg::get()`/`jmg::try_get()` to return non-`const`
    objects/proxies to allow JMG objects to be treated consistently
    with e.g. `std::tuple` and `std::variant`

## Native and CBE objects

* Represent JMG `Union` type using `std::variant`?
  * Try fleshing out the design in test_native.cpp

## Protobuf objects

* Rename protobuf header file as **jmg/proto/proto.h** instead of
  **jmg/protobuf/protobuf.h**?
  * Same for the `protobuf` namespace?
  * This will allow the jmgc encoding name to match the directory and
    file name, instead of having protobuf as a one-off difference
    compared to the other encodings.
* Protobuf 'bytes' type should work with BufferView?
  * Maybe all objects need general BufferView support?
* Support repeated enum fields
* Support repeated `TimePoint` fields
* Support repeated safe type fields
* Prohibit use of non-protobuf enums and scoped enums as fields for
  protobuf objects
* Fix perfect forwarding in `setByType`
* Support `oneof` fields using `jmg::Union`
  * May not be necessary but could add some checking on the `set()`
    case?
  * Does seem necessary to generate a `oneof` in a .proto
  * See design example at the bottom of test_protobuf.cpp
* Modify jmgc to generate .proto files using JMG YAML spec as the
  source of ground truth
* ~~Support special handling of time points~~
* ~~Support single class fields~~
* ~~Support repeated string fields~~
* ~~Support enums~~
* ~~Support safe types~~
* ~~Support repeated class fields~~

## YAML objects

* Correctly support StringField instead of allowing FieldDef<string...>

# Ongoing/interrupted project notes

## support for flatbuffers

This is extremely hairy since flatbuffers has no reflection interface
like protobuf does. Flatbuffers `FieldDef` declarations may need to
include retrieval functions via lambda or some other mechanism. Seems
like a place where macros would be beneficial and there would be tight
coupling between the _field_ and _object_ definitions.

Another alternative would be to break open the serialization encoding
and tie the _object_ implementation to it, but this seems excessive...

## Misc Standardization

* ~~Use `JMG_TAG_TYPE` to declare all tag types~~
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
