# Summary

Experimental library that seeks to express some new ideas.

* Use metaprogramming and other techniques to leverage the compiler
  for greater code safety.
  * The goal here is to build confidence that, if the code compiles,
    then it will run correctly.
  * While this goal is not attainable in general, the closer we can
    get, the better.
  * c.f. the principle of _shift left_
* Users should be heavily rewarded for becoming familiar with the code base.
  * Common/standard idioms are defined and used consistently
  * Standard abbreviations are defined and used consistently

This is the opposite of the philosophy that code should be 'simple' so
that any user can quickly start using it. The 'simple' approach, in my
experience, produces an outcome best described as _piles and piles of
code_, where there is no attempt to create coherency and usage
patterns are irregular and unpredictable.

The approach taken here is that libraries should be designed as
something akin to _domain-specific languages_ with powerful
vocabularies to express solutions clearly and enable the reader of the
code to quickly grasp its function

# Glossary

Jormungandr strives to be precise (or maybe pedantic) with its
vocabulary, here are some terms that are used in the code and this
documentation whose meaning may not be immediately obvious.

* _safe type_ - AKA "strongly-typed alias", see the section on _Safe
  Types_ Framework under Subsystems for more information
* _owning type_ - a type that owns resources, e.g. `std::string`,
  `std::vector`.
* _view type_ - a facade over an owning type that provides lightweight
  copy/by-value functionality along with a degree of type erasure when
  compared to using references, e.g. `std::string_view`,
  `std::span`. Additionally, Jormungandr tries to consistently present
  _view types_ as not being able to modify the contents of the objects
  that they view, although this consistency is still a work in
  progress.
* _proxy type_ - like a _view type_ but capable of modifying the contents
  of the referenced object. As with _view types_, consistency is a
  work in progress and some types purporting to be _view types_ are
  actually _proxy types_, although there are no known cases of _proxy
  types_ that cannot modify the referenced object.
* _viewable type_ - somewhat synonymous with _owning type_ but focuses
  on the fact that there is an existing associated _view type_, and
  possibly _proxy type_.
* _array type_ - basically any language or standard library type that
  can be viewed/proxied with `std::span`:
  * C-style array
  * `std::vector`
  * `std::array`
* _string type_ - any type that can be viewed with `std::string_view`,
  can also be viewed conceptually as a subset of _array types_:
  * C-style string
  * `std::string`
* _range type_ - the set that contains both _array types_ and _string
  types_. Conceptually isomorphic to the `std::ranges::range` concept
  from c++20, although perhaps not exactly equivalent in practice.
* _dictionary_ - here's one thing python got right: using the name
  _dictionary_ instead of _map_ for collections of `(key, value)`
  bindings. _dictionary_ is consistent with the nomenclature used by
  theorists and allows _map_ to refer to what is currently called
  _transform_, and I strongly prefer _map_/_reduce_ to
  _transform_/_accumulate_.
  * The go-to _dictionary_ container class template in Jormungandr is
    called `Dict`. This is (currently) an alias for
    `absl::flat_hash_map` so the interface is highly consistent with
    `std::unordered_map` and I don't expect that to change (i.e the
    implementation that `Dict` is aliased to might change at some
    point but it would never be backed by a structure whose interfaces
    depart substantially from `std::unordered_map`; those would likely
    not be aliased unless the original names were exceptionally
    obnoxious).
  * For _dictionary_ containers that require ordered traversal, the
    appropriate class template is called `OrderedDict`, which is
    (again, currently) backed by `absl::btree_map`.
  * As an aside, there are also `Set` and `OrderedSet` aliases with
    characteristics predictable by analogy to `Dict` and `OrderedDict`
* _RPC_ - AKA _Remote Procedure Call_ this term encompasses any
  interaction with a remote host in which each outgoing request is
  expected to receive a finite set of responses. This covers
  activities defined using a diverse set of protocols including HTTP
  and ODBC as exemplars of internet protocols and database protocols,
  respectively.

# _Avant Garde_ Techniques

## Return Type Overloading

Non-trivial type conversion is very messy in c++. There are multiple
generations of conversion functions in the standard library that have
incompatible interfaces, provide varying levels of performance and
often require overspecification (especially via poor-quality
techniques such as embedding the name of a type in the name of a
function, e.g. `to_string`, which complicate generic programming
unnecessarily). At some point, I stumbled across [a
technique](https://artificial-mind.net/blog/2020/10/10/return-type-overloading)
that produces (at least the illusion of) return type overloading that
appealed to my desire for terse code. I decided to experiment with the
approach despite the statement by its author that it should not be
taken as "how to use return-type overloading in production" and
ignoring the caveats about how this can go wrong. The result is a very
terse interface consisting of a family of variadic function templates
named `from`. The use of this is very simple: call `from` on a
variable that should be converted to another type (typically, but not
always, a string) and assign the result to another variable or pass it
as a function argument. For example:

```
double dbl = jmg::from("0.5");

void doSomethingWith(uint32_t value);
// some time later...
const auto strToConvert = "42"s;
doSomethingWith(from(strToConvert));
```

The result is what one would expect from a plain reading of the code:
the value is converted to the desired type without requiring explicit
specification in the call (such as `from<int>(str)`). Trying to
execute an unsupported conversion will fail to compile, as it
should. There are some quirks here: it is one of the very few things
that will cause me to depart from my usual _auto all the things_ style
(since `auto val = from(str);` will fail to compile because there is
no way to know what type to convert the value to), and passing the
result of calling `from` to function templates will sometimes require
the addition of `static_cast` to disambiguate the result.

The following single-argument (to `from`) conversions are currently
supported:

* `std::string` to...
  * string (to handle degenerate cases in generic code)
  * numeric/arithmetic types
* `std::string_view` or `const char*` to anything that `std::string`
  can convert to
* back and forth between various time point types
  * `jmg::TimePoint` (AKA `absl::Time`)
  * `time_t` (AKA _seconds since unix epoch_)
  * `timeval` (`time_t` plus micros since second)
  * `timespec` (`time_t` plus nanos since second)
  * `boost::posix_time::ptime`
  * `std::chrono::time_point<std::chrono::system_clock>`
* back and forth between various time duration types
  * `jmg::Duration` (AKA `absl::Duration`)
  * `std::chrono::duration`

The addition of one or more arguments (format spec and optional time
zone) will allow conversion between string and `jmg::TimePoint`, where
the format spec is the same as per the time parsing/formatting support
from `absl::Time` (which provides the underlying implementation).

## Named Value Parameters

OK, the name here is a bait-and-switch because it doesn't provide
named value parameters for functions in the way that python does (go
define a `struct` and use designated initializers if that's what
you're after). Instead, the idea here is to allow the definition of
variadic function templates that can take some or all of their
parameters in any order and will figure out at compile time whether or
not the call is correct (i.e. doesn't involve mutually exclusive
parameters, is not missing some required parameters, etc). The
technique depends on the types of the various parameters being
distinct, with _safe types_ used to ensure this property. Under the
hood, it's powered by a lamda, a fold expression, some metaprogramming
and a generous helping of `if constexpr`.

## Template Policy Framework

**TODO**

## Documentation placeholders
### Command Line Parameter Handling

**TODO**

# Major Frameworks and Subsystems

## _Safe Types_ Framework

I first had the idea for this when reading about the failure of [the
Mars Climate Orbiter](https://en.wikipedia.org/wiki/Mars_Climate_Orbiter),
where it was obvious that encoding something about the sematics of the
value in its type would have prevented the bug from getting past the
compilation stage. At the time, boost.units seemed like the right
solution, but investigating it further revealed 2 issues:

* There is a lot of complexity related to conversions between
  different representations of a quantity. If you are trying to
  calculate the difference between 2 distances where one is specified
  in millimeters and the other is specified in angstroms, then this is
  the right library, but it's overkill for my purposes.

* It is very difficult to create new systems using the tools
  available. I tried several times and was unable to make it work.

I subsequently realized that what was really needed was to carefully
separate values with different semantics, not safely intermix
them. For this approach, it's sufficient to employ strongly-typed
aliases so that, for example, when populating option risk data, you
don't accidentally pass a Delta value where the system is expecting a
Vega. To reach this goal, it's sufficient to have strongly-typed
aliases (which c++ doesn't have out of the box) and there are several
libraries available which implement this functionality, so I chose
[what I found to be the best one](https://github.com/doom/strong_type)
and based my solution on it.

### Declaring a _Safe Type_

**WARNING:** this implementation is currently affected by a
long-standing bug in gcc where the compiler fails to ensure that all
labmda constructs have different types, including the trivial lambda
`[](){}`. This bug induces a requirement to use a macro to declare
safe types but hopefully gcc will fix this problem and the macro can
be removed to allow a simpler declaration.

A _safe type_ is essentially a compile-time wrapper around another
type that prevents intermixing values of the _safe type_ with the
_wrapped_ type. In addition, a _safe type_ can declare that certain
operations can be performed between values of the type, but any
operations not declared _safe_ are forbidden by the compiler. Here is
an example of declaring a _safe_ identifier that is represented as a
32-bit unsigned integer:
```c++
using SafeId32 = jmg::SafeType<uint32_t,
    st::equality_comparable, st::orderable, st::hashable>
```
Note that use of the `st` namespace is an artifact of the underlying
library and may be eliminated at some point. Declaring the type to be
`st::equality_comparable` allows 2 IDs to be compared with
`operator==`, while `st::orderable` permits use of inequality
operators such as `operator<`, and `st::hashable` enables the use of
`std::hash` on the type. Basically, this declaration allows `SafeId32`
to be used as the key type in ordered and unordered dictionaries. For
the moment, you can check the documentation for the underlying library
to see what other operations are available. The only other important
aspect of _safe types_ is the ability to extract the _wrapped_ type
value using the _unsafe_ function so that it can be used to interact
with other types where necessary. This facility should be used very
sparingly, and the lifetime of the unwrapped value should be
constrained as much as possible.

## Simple Server Framework

It turns out that there are some complex and subtle points involved in
creating a robust server, mostly revolving around signal handling. In
order to avoid having a choice between a flaky server and copying a
bunch of boilerplate code, I created a framework that wraps `main` and
performs all of this work automatically, leaving the developer to
simply create a subclass of the `Server` class in the `jmg` namespace,
implement a couple of member functions and register the class with a
simple factory mechanism. The required member functions are named
`startImpl` and `shutdownImpl`, and the base class is pure virtual so
the compiler will require them to be overridden. As one might expect,
`startImpl` is where the initialization code will go, and the server
will continue waiting for a shutdown signal even if it
returns. `shutdownImpl`, of course, should execute the shutdown and
cleanup code, and it will be called automatically when a shutdown
signal is received. The expected shutdown signals are `SIGTERM` and
`SIGINT`; all other signals are ignored where possible. Once the
`Server` subclass has been defined, it is necessary to call the
`JMG_REGISTER_SERVER` macro on it so that the framework can start it
automatically. The last thing to remember is that adding
`//:jmg_server_main` to the `deps` of the bazel `cc_binary` target for
the server will cause it to be linked auatomatically against the
library that declares the `main` function. A simple demo of this
framework can seen in the file `test/test_server_main.cpp`.

## _Standard Interface_ for _Objects_

In this case, _objects_ can be thought of as hierarchical collections
of key/value bindings. Many concepts can be viewed in this way:
* Command line parameters (flat collection, not hierarchical).
* Configuration settings read from files using formats such as XML or YAML.
* Messages used to communicate between separate processes, either
  locally or as part of a distributed system.

The first of these (command line paramters) has already been
completely converted to the _standard interface_ style, mostly because I
was deeply dissatisfied by the available options for solving this
problem.

There is also support for accessing data from XML and YAML files using
the _standard interface_ style, although it has not yet been used
specifically for configuration files. This will probably also be
extended to JSON at some point as well.

The third target, messages used for communication, is the most
important. One of the most painful parts of writing code to
communicate between processes is the proliferation of message
encodings, each generally with its own interface. To be frank, most or
all of these interfaces are of poor quality and there are numerous
styles of interaction, which produces severe impediments to the
ability to switch between them. The goal of the _standard interface_
is to present a facade on top of these interfaces to unify them into a
single style of interaction with maximal (compile time) safety
features and minimal runtime overhead. Whether data is encoded as
protobuf or JSON, FIX protocol or rows from a database or any of the
numerous other encodings out there, the _standard interface_ coding
style should be the same, and the code should look exactly (or almost
exactly) the same in situations where the data has fields with the
same semantics.

The basic groundwork has been laid for handling messages in the
_standard interface_ style, and substantial work has been done on
processing the following formats:
* FIX protocol - via the XML configuration format used by the QuickFIX
  library (commonly used in the financial industry). Mostly proof on
  concept that may eventually be extended to produce JMG object
  wrappers for QuickFIX objects.
* YAML - currently used as the basis for the IDL in which JMG object
  specifications are written. All object access is currently
  read-only.
* CBE - encoding for **native** JMG objects that also serves as a
  testbed for extending and reworking the _standard interface_, as
  well as (eventually) for experiments related to performance of
  binary encodings.

### Interface Definition Language

It is complex and difficult to express interfaces in the raw building
blocks of the _standard interface_ style, and some of the potential
encodings also use IDL compilers to generate code (e.g. protobuf),
which is a very effective technique. In order to simplify generation
and conversion of interfaces, Jormungandr has a simple IDL called
JMG that is specified in YAML to avoid needing to write a separate
parser, and a compiler named **jmgc** that generates 'stubs'.

#### JMG IDL Language Specification

**TODO**

#### jmgc - IDL Compiler

The compiler generates definition header files from IDL specifications
written as YAML files because that format is serviceable (if a bit
clunky) and it avoids the need to write a parser.

**jmgc** currently
works for definitions written in the JMG language (although this is
still very much a work in progress), and also has limited support for
QuickFIX XML definitions (written as a proof of concept without much
support or testing for intracting with actual FIX messages).

One idea under consideration here is to have **jmgc** support
generating specifications in other IDLs where Jormungandr has support
for wrapping their native objects. An example would be protobuf and
the idea is to have _standard interface_ wrapper support for protobuf
objects and **jmgc** support for generating `.proto` files. This would
allow the source of ground truth for the system protocol to be JMG
language and would be valuable even if Jormungandr has direct
implementation of serialization and deserialization of protobuf wire
format (under consideration) because protoc could still be used to
generate the bindings for other languages such as Java or Python.

At this point, **jmgc** generates the following types of wrappers:

* FIX protocol object wrappers using specifications written in the XML
  configuration files used by the QuickFIX library.
* YAML object wrappers specified in the JMG language.
* CBE object wrappers specified in the JMG language.

### TODO

* A _standard interface_ for stream-based processing of messages that
  is consistent with the handling of object representation
  * The modivation here is the SAX approach to XML handling, as
    compared to the DOM approach, which would then be analogous to the
    existing implmementation that represents fully parsed/deserialized
    objects
* Creation and update of objects using the _standard interface_
* Finish parsing and generation for SBE specification (some partial
  work has been done here)
* Develop a plugin architecture for **jmgc** to simplify adding
  support for new _standard interface_ wrappers and other IDLs?
* Support for more encodings such as protobuf and JSON

## **Native** Encoding

### Internal Format

Internally, **native** encoding of objects consists of `std::tuple`
wrapped in an interface that supports the _standard interface_ verbs.

Care has been taken to deal in _view_/_proxy_ types wherever
appropriate and the hope is that coupling this approach with lots of
opportunities for the compiler to find optimizations in templated code
will produce high performance results.

### Serialization Format

The **native** encoding is paired with a serialization format named
**Compressed Binary Encoding** (AKA **CBE**) that attempts to mix
Google protobuf _varint_ with FIX-FAST generic stop bit encoding and add
in support for compressing floating point numbers. Difficult to say
whether the floating point compression will have value because it
mostly depends on how much precision is being stored in any given
application, although having a lot of zeros will compress well. This
will also serve as a test bed for some features of interest:

* Supporting separate object-based and stream-based decoding
  algorithms. These are analogous to DOM style vs SAX style parsing in
  XML.
* Using _view_/_proxy_ types wherever appropriate/possible.
* Finding a way to integrate polymorphic memory allocators.
* Handling _array types_ and _string types_ with the same code where
  possible. Perhaps this will involve designing concepts that form a
  proper hierarchy for _owning types_ as compared with _view_ or
  _proxy_ types.

#### TODO list for **CBE**

* Add **CBE**/**native** support to **jmgc**.
* Continue developing support for _viewable types_
  * Improve _viewable type_ handling to support 'pmr'
  * Export _viewable type_ handling to other encodings
* Add exhaustive tests of cases where buffers are too small to encode
  or have been truncated before decode
* Add some metaprogramming to constrain fields to contain only
  allowable types, especially which types are allowed to be in arrays
* Rethink and rework buffer handling. The current implementation only
  works with fixed-size buffers represented using `std::span`, which
  will complicate things when encoding with sub-objects (due to the
  need to prefix the length) and will prevent use of multiple buffers
  where objects cross buffer boundaries. `absl::cord` could form the
  basis for an initial implementation, but inspiration should be drawn
  buffer implementations in the standard streaming libraries as well
  as _boost.asio_ buffer handling.
* Review google protobuf wire format to see how hard it would be to
  modify CBE code to serialize and deserialize protobufs to
  jmg::native objects.
* Experiment further with serialization/deserialization.
  * Create an interface for stream deserialization with externally
    accessible callbacks and experiment to determine whether this
    should be the basis for object deserialization or a separate mode.
  * Does it make sense to enable flexibility by encoding a type
    specifier into the field ID so that unkown fields can be cleanly
    handled? Maybe this can be done with only 2 bits to minimize the
    overhead for small messages.
  * Maybe it should go the other way and use an presence map along
    with explicit versioning and possibly support for forward/backward
    compatibility and/or version negiotiation built into communication
    channels
  * Maybe implement the verbose, flexible approach (with embedded type
    specifiers) and the rigid presence map and use these together with
    the existing implementation to explore the performance tradeoffs.
* Rethink floating point compaction: the current implementation isn't
  very effective (e.g. the default encoding of 42.0 requires 10
  octets).
  * There are extra bits available in the exponents to allow the
    addition of another metadata bit that would indicate whether or
    not the mantissa was compressed: if the bit is set, the mantissa
    is compressed with stop-bit encoding and should require fewer
    octets than the native encoding; otherwise, the mantissa octets
    should be copied without change.

# Type Library

This consists of various useful types that are generally similar to
existing types but with some improvement.

## Time Points and Durations

### `jmg::TimePoint`

Internal representation of a time point that is convertible to and
from other common time point implementations, as described earlier in
the documentation for `from`.

The current system time can be retrieved in this format by calling
`jmg::getCurrentTime()`.

### `jmg::EpochSeconds`

_Safe type_ wrapper around the POSIX `time_t` type that represents the
number of seconds since the UNIX epoch (1970-01-01).

#### `jmg::TimePointFmt`

_Safe type_ wrapper for a string that describes the format used to
parse and format time points as strings. See [the documentation on
formatting Abseil
Time](https://abseil.io/docs/cpp/guides/time#formatting-absltime)
along with [the comments on the FormatTime() function in the time.h
header
file](https://github.com/abseil/abseil-cpp/blob/76bb24329e8bf5f39704eb10d21b9a80befa7c81/absl/time/time.h#L1469-L1504)
for details.

There are also constants for common formats:
* `jmg::kIso8601Fmt` - [ISO
  8601](https://en.wikipedia.org/wiki/ISO_8601)/[RFC3339](https://datatracker.ietf.org/doc/html/rfc3339)
  format, without appended time zone specifier
* `jmg::kIso8601WithZoneFmt` - [ISO
  8601](https://en.wikipedia.org/wiki/ISO_8601)/[RFC3339](https://datatracker.ietf.org/doc/html/rfc3339)
  format, with appended time zone specifier

### `jmg::TimeZone`

Internal representation of a time zone. The object for the UTC time
zone can be retrieved by calling `jmg::utcTimeZone()`

#### `jmg::TimeZoneName`

_Safe type_ wrapper for a string containing a TZ database time zone
name (c.f. [the list of TZ database time
zones](https://en.wikipedia.org/wiki/List_of_tz_database_time_zones)).

To get the time zone object corresponding to a given name, call the
`jmg::getTimeZone()` function.

### `jmg::Duration`

Internal representation of a time duration that is also convertible to
and from other common durations, per the `from` documentation.

## `jmg::c_string_view`

This is almost exactly like `std::string_view`, except that it is
guaranteed to be a view on a string that is terminated by a `NUL`
byte, and it consequently provides a `cstr()` member
function. Generally useful when dealing with older interfaces that
expect C-style strings. Defined in **include/jmg/types.h**

## `jmg::Promise`/`jmg::Future`

These are wrappers around the existing `std::promise`/`std::future`
classes with some minor improvements:

* For an instance of `jmg::Promise`, if no value has been set and no
  exception was previously captured, and its destructor determines
  that an exception is in flight, it will automatically capture that
  exception so that it will be delivered by `Future` instead of
  `std::broken_promise`
* There is an overload of `jmg::Future::get` that takes a _duration_
  parameter and an optional description parameter where the _duration_
  is a timeout value; if no value is available before the timeout
  expires, the function will throw an exception instead of requiring
  the caller to use a separate `wait_for` function to enforce a
  timeout

# Utility library

## Random Number Generation

To generate random integers over a range with a starting and ending
value using a uniform distribution, instantiate an object of type
`jmg::RandomInRange`, which is a class template parameterized on the
(integral) return type whose constructor takes the starting and ending
ranges, and then call its `get()` member function. Defined in
**include/jmg/random.h**.

**WARNING:** `jmg::RandomInRange` objects are not thread safe.

**TODO** Add support for generating random floating point numbers.

**TODO** Add support for random number distributions other than
  uniform?

# Coding Standards

A corollary to the philosophy of expecting that users will be familiar
with the code base is the requirement to make the familiarization
process as easy as possible through documentation. This section
contains a documentation of some of the standards associated with the
project, especially those that may differ from expected practice.

In general, code should follow the C++ core guidelines, with a few
exceptions and additions as outlined here

## General Principles

These may or may not be controversial but they have served me well.

* If it can be `const` then it should be `const`
* Impute semantic meaning to types using the _safe types_ framework
* Use metaprogramming to make interfaces more robust
* **Always** use exceptions, and rely on RAII for error handling

### Time Point and Duration Handling

Internally, time points should **always** be represented using
`jmg::TimePoint` and `jmg::Duration`, respectively. `jmg::TimePoint`
should be used for timeout deadlines and `jmg::Duration` should be
used for timeout durations. In particular, time zones and human
readable timestamps are presentation issues and should be pushed as
far to the edges of a system as possible by parsing incoming strings
before injecting them as well as formatting outgoing strings as late
as possible. The internal time representation is effectively in the
UTC time zone, but this detail should never be relied on directly, and
time points should be viewed as black boxes.

### Floating Point Number Handling

There several important rules for handling floating point numbers:

* Never, **EVER** treat the value **NaN** as an equivalent to **NULL**
  (i.e. no value present), *always* use `std::optional` to handle
  situations in which a value may or may not be present. **NaN**
  specificially represents a number that cannot be represented in the
  given encoding (typically [IEEE
  754](https://en.wikipedia.org/wiki/IEEE_754)), and can be the result
  of a computation. Many systems have made the mistake of conflating
  **NaN** with **NULL**
  (e.g. [kdb+](https://code.kx.com/q/interfaces/capiref/#constants)
  and
  [pandas](https://pandas.pydata.org/docs/user_guide/integer_na.html)),
  and it always ends in tears. This is the most important rule for
  dealing with floating point numbers due to frequency with which it
  is violated and the (long-term) consequences for doing so,
  regardless of the emphasis more commonly placed on the following
  rule.
* Always use a tolerance (AKA _epsilon_) when comparing floating point
  numbers. Everyone knows this, the rule is just to remind you.
* Not commonly known: floating point numbers have a concept of
  [normality](https://en.wikipedia.org/wiki/Normal_number_(computing)),
  which is represented in C/C++ using
  [std::isnormal](https://en.cppreference.com/w/cpp/numeric/math/isnormal.html). One
  subtle and confounding point of this concept is the fact that **0 is
  not considered a 'normal' number**. You should be aware of this if
  you ever need to use `std::isnormal`.
* Also not commonly known: [NaN
  values](https://en.cppreference.com/w/cpp/numeric/math/nan.html)
  (i.e. `std::nan`, `std::nanf` and `std::nanl`) cannot be compared
  directly for equality, such comparisons (e.g. `if
  (some_value_produced_by_a_computation == std::nan)`) will **always**
  return `false`. In fact, as can be seen by the fact that functions
  for creating **NaN** values take `const char*` parameters, there are
  many types of **NaN** values, but the best you can hope to
  accomplish is to use
  [std::isnan](https://en.cppreference.com/w/cpp/numeric/math/isnan.html)
  to determine if a floating point value does not represent an actual
  number.

## Helpers

There are a few helper functions used in the code to simplify certain
awkward interactions that regularly occur when using standard library
functions and classes:
* key_of - get the `key_type` from the `value_type` object stored in a
  dictionary
* value_of - get the `mapped_type` from the `value_type` object stored
  in a dictionary
* always_emplace - calls `try_emplace` on a dictionary and throws an
  exception if the insertion wasn't successful (i.e. the key is a
  duplicate of one that already exists in the dictionary)
* always_insert - calls `insert` on a set and throws an exception if
  the insertion wasn't successful (i.e. the item is a duplicate of one
  that already exists in the set)
* as_void_ptr - takes a pointer argument and converts it to a
  non-`const` `void*`, which is useful when dealing with some
  low-level code.

## Idioms

There are some idioms that are used regularly in Jormungandr code but
are less common or non-existent in other codebases.
* _ENFORCE_ macros - there is a series of macros that simplify various
  situations where the failure of a predicate should lead to an
  exception being thrown. Some examples are as follows:
  * `JMG_ENFORCE` - first argument is a `true`/`false` predicate and
    the second is a streamed string that supplies an error message. If
    the predicate fails, the string is used to construct an instance
    of `std::runtime_error`, which is then thrown
  * `JMG_SYSTEM` - the solution for the annoying POSIX interface where
    many functions return an `int` that is set to `-1` on failure, with
    `errno` being set to provide more information. `JMG_SYSTEM` is
    very similar to `JMG_ENFORCE` but the result of the function call
    is compared vs `-1` internally and there is embedded to to
    automatically extract `error` and use it, along with the supplied
    message, to throw an instance of `std::system_error`. There are
    also several variants of this to handle other common patterns of
    POSIX failure reporting bone-headedness.
* _Immediately Invoked Lambda Expression_ (IIFE) - in this idiom, an
  unnamed lambda is defined and immediately executed, with its result
  assigned to a variable or passed as a function argument. The purpose
  here is to preserve `const`-ness of automatic variables and
  generally reduce the clutter of extraneous variables in situations
  where using a scope does not suffice.
* _Store And Forward_ POSIX calls - an idiom layered on top of the
  POSIX-specific _ENFORCE_ macros where an _Immediately Invoked Lambda
  Expression_ that captures some previously declared variables by
  reference is used as the function argument to the _ENFORCE_ macro to
  handle the situations where the return value is useful in the
  non-failure case. (e.g. `open` which returns `-1` on error and the
  file descriptor associated with the opened resource otherwise). In
  this idiom, the return value is stored to a captured reference,
  whose value is also returned from the lambda, which allows the macro
  to do its work while leaving the value available for later use if no
  failure is detected.

## Standard Abbreviations

Naming is well known to be one of the hard problems in software
engineering. To facilitate the appropriate naming of things while
keeping the code compact, a series of standard abbreviations is
catalogued here. If you see an abbreviation whose meaning is not
obvious, look in this section for a defintion. When contributing code,
please use these abbreviations where appropriate.

* rslt  - result, typically used to name an automatic variable that
  will return a value from a function so as to take advantage of named
  return value optimization
* chkpt - checkpoint
* cncy  - currency
* ctrl  - control/controller
* ctxt  - context
* def   - definition
* fbr   - fiber
* fld   - field
* msg   - message
* px    - price
* rc    - return code, typically used for POSIX functions and other 3rd
          party library functions that insist on returning an integer code
* req   - request
* rsp   - response
* srvr  - server
* str   - string
* strm  - stream
* svc   - service
* sz    - size/length/count
* tp    - time point
* ts    - timestamp

## Some Other Naming Standards

### Common Prefixes

These prefixes are intended to allow intent/function to be easily
discerned when reading code

* try - function name prefix indicating that the function will
  attempt to perform some action that results in a value being
  returned where it is not a failure if no value can be returned. The
  return value of such a function will be a specialization of
  `std::optional` or may be `bool` to indicate the success of the
  attempted action. Note that, in the case of a `bool` return type, a
  return value of `false` indicates _non-success_, and is considered
  to be distinct from _failure_, which is communicated by exception in
  all cases.
* maybe - function name prefix indicating that the function will
  attempt to perform an action but whether or not the action has
  actually been performed is irrelevant to subsequent program
  logic. The most common example of this is `maybeInitialize()`, whose
  name expresses the intent that the function will perform some
  initialization action the first time it is called and will otherwise
  do nothing.

# Some Longer-term Goals

## High Performance Event Processing

Not much work here yet, but the idea is to mix the _standard
interface_ style of messaging with a high performance event loop based
on fibers and io_uring on Linux. This is a big lift, and the only work
done so far is a bit of exploration and prototyping to try to find a
clean interface that is consistent with a specific internal vision of
how it should look from the user perspective. I may try to create
something using existing boost tools that will produce the desired
user-facing style and then work to steadily swap out the internals for
improved performance.

NOTE: this work is currently under way in the experimental/reactor
directory

# Interesting Ideas That May Be Developed Further

## Systematically Handle Currency Values With Proper Base Units

The goal is to avoid the use of floating point for prices (i.e. use
cents instead of dollars as the base unit for USD prices). This idea
is drawn from boost.units although it probably won't be productive to
try to create another unit system for boost.units.

## Systematically Handle a Multi-Currency Environment.

Is there an efficient way to tag the currency value (i.e. price) with
currency it is being expressed in? Would it make sense to use safe
types to encode the currency for a price into the type system?

# Other Overall TODO Items

* Add test cases for code that should fail to compile using
  `cc_build_error` rules from `rules_build_error`
  * There appear to be some bugs in `rules_build_error` that need to
    be worked out, an initial attemp at adding this functionality
    failed.
* Standardize naming of all concepts and type metafunctions
* Document coding standards for concepts and type metafunctions
