
# Cleaned up support for JMG _field_ and _object_ definitions

## Goal

All JMG _object_ support should use consistent naming and structure
supported by consistent behavior in the basic libraries.

Support structures should be documented along with the procedure for
using them to create new _object_ types.

## Issues

### Handling of _forcible argument and return type_ declaration

Proper function argument and return type handling is complicated by
the properties of the many types that must be supported. A (hopefully)
complete case analysis is as follows:

* Primitive types should be passed and returned by value.
* Non-primitive types should be passed and returned by reference
  * References to non-primitive types will always be `const` in the
    current implementation, but this may be relaxed to allow `const`
    or non-`const` based on context in the future.
* Some non-primitive types are _viewable_.
  * _viewable_ -> can be represented by either `std::string_view` or
    `std::span` (AKA _view_ types).
  * Field definitions of _viewable_ types are currently handled
    separately via the `StringField` and `ArrayField` declarations
  * _View_ types should always be returned by value, but it would be
    good to support passing either the _view_ type by value or the
    related _viewable_ type by `const` reference.
  * Standard library constructs relate to _view_ types as follows:
    * `std::string_view` can represent any string type, including
      `std::string` and _C-style_ string.
    * `std::span` can represent any non-string type, including
      `std::vector` and `std::array`.
  * It is also desirable to represent non-standard constructs using
    standard library _view_ types when/if possible.
  * _View_ types represent a form of type erasure and it is desirable
    to use them whenever possible to avoid template creep.
* JMG has a separate concept of a _proxy_ type (currently exemplified
  by `jmg::ArrayProxy`).
  * _Proxy_ types are similar to _view_ types but they have 2 features
    that are distinct:
    * possible _ownership_ of resources separate from the underlying
      object(s)
    * Required _adaptation_ of underlying object(s) to allow
      interaction with them via standard idioms such as `ranges`.
  * _Proxy_ types should be passed and returned by value, even when
    they involve _ownership_ of separate resouces (which should be
    minimized).
* Support for JMG _safe_ (AKA _strongly aliased_) types is highly
  desirable but complicated.
  * _Safe_ types should be returned by value or by reference as if
     they were the non-_safe_ type that underlies them.
  * This should include _proxy_ types
  * Arrays of _safe_ types are particularly complicated, can support
    be seemlessly achieved via a _proxy_ type that adapts retrieval?
  * Declaration of _safe_ string types is hampered by the following
    problems:
    * Inability of `StringField` to support declaration of a related
      safe type
    * Inability of a _safe_ type with `std::string` as the underlying
      type to be represented by `std::string_view`
* Handling of _Optional_ fields retrieved by `jmg::try_get()` is more
  complicated than it initially appears.
  * An _optional_ primitive type `T` should be returned via
    `std::optional<T>`
  * A _safe_ _optional_ primitive type `T` with underlying type `U`
    should be returned via `std::optional<T>`
  * An _optional_ non-primitive type `T` should be returned via `const
    T*`
  * A _safe_ _optional_ non-primitive type `T` with underlying type
    `U` should be returned via `const T*`

To summarize the results of the above analysis:

* primitive type `T`
  * non-specialized
    * return by value `T`
    * pass by value `T`
  * _safe_
    * return by value `T`
    * pass by value `T`
  * _optional_
    * return by `std::optional<T>`
    * pass by `T`
  * _safe_ _optional_
    * return by `std::optional<T>`
    * pass by `T`
* non-primitive type `T`
  * non-specialized
    * return by `const T&`
    * pass by `const T&`
  * _viewable_ type `T` with _view_ type `V`
    * return by value `V`
    * pass by value `V`
    * pass by `const T&`
  * _proxied_ type `T` with _proxy_ type `P`
    * return by value `P`
    * **?? pass by value `P` ??**
    * pass by `const T&`
  * _optional_ non-specialized type `T`
    * return by `const T*`
    * pass by `const T&`
  * _optional_ _viewable_ type `T` with _view_ type `V`
    * return by `std::optional<V>`
    * pass by value `V`
    * pass by `const T&`
  * _optional_ _proxied_ type `T` with _proxy_ type `P`
    * return by `std::optional<P>`
    * **?? pass by value `P` ??**
    * pass by `const T&`

In order to facilitate proper type conversions, a set of
metaprogramming constructs will be used. These will be declared in
**meta.h** and in order to properly support all the requirements,
**meta.h** should depend on **safe_types.h** and **array_proxy.h**.

#### Open questions

* Which (if any) known types not in the standard library can be
  represented by standard library _view_ types?
* Can _view_ and _proxy_ types be treated consistently?
* Can _safe_ types with `std::string` as the underlying type be
  represented by `std::string_view`?

# The plan

* Make **meta.h** depend on **safe_types.h** and **array_proxy.h**
* Standardize naming and namespace interaction for _field_ and
  _object_ types of all encodings.
