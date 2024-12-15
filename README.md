# Summary

Experimental library that seeks to express some new ideas.

* Use metaprogramming and other techniques to leverage the compiler
  for greater code safety.
  * The goal here is to build confidence that, if the code compiles,
    then it will run correctly.
  * While this goal is not attainable in general, the closer we can
    get, the better.
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

## Documentation placeholders
### Return Value Overloading
TODO
### Standard Interface for Objects
TODO
### Command Line Parameter Handling
TODO

# Subsystems

## _Safe_ Types Framework

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

### Declaring a _Safe_ type

A _safe_ type is essentially a compile-time wrapper around another
type that prevents intermixing values of the _safe_ type with the
_wrapped_ type. In addition, a _safe_ type can declare that certain
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
aspect of _safe_ types is the ability to extract the _wrapped_ type
value using the _unsafe_ function so that it can be used to interact
with other types where necessary. This facility should be used very
sparingly, and the lifetime of the unwrapped value should be
constrained as much as possible.

### TODO document the _named parameter_ idiom

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
framework can seen in the file test/test_server_main.cpp.

# Some longer-term goals

## _Standard Interface_ for _Objects_

In this case, _objects_ can be thought of as hierarchical collections
of key/value bindings. Many concepts can be viewed in this way:
* Command line parameters (flat collection, not hierarchical).
* Configuration settings read from files using formats such as XML or YAML.
* Messages used to communicate between separate processes, either
  locally or as part of a distributed system.

The first of these (command line paramters) has already been
completely converted to the _standard interface_ style, mostly because I
was deeply dissatisfied by the available options for solving these
problems.

There is also support for accessing data from XML and YAML files using
the _standard interface_ style, although it has not yet been used
specifically for configuration files.

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
protobuf or json, FIX protocol or rows from a database or any of the
numerous other encodings out there, the _standard interface_ coding
style should be the same, and the code should look exactly (or almost
exactly) the same in situations where the data has fields with the
same semantics.

The basic groundwork has been laid for handline read-only messages in
the _standard interface_ style, and substantial work has been done on
building the facade for FIX protocol as the first implementation,
although it hasn't yet been used to wrap the QuickFIX interface
(commonly used in the financial industry)

### Interface Definition Language

It is complex and difficult to express interfaces in the raw building
blocks of the _standard interface_ style, and some of the potential
encodings also use IDL compilers to generate code (e.g. protobuf),
which is a very effective technique. In order to simplify generation
and conversion of interfaces, Jormungandr has a simple IDL called
JMG that is specified in YAML to avoid needing to write a separate
parser, and a compiler named **jmgc** that generates 'stubs'.

#### JMG IDL language specification

**TODO**

#### jmgc - IDL compiler

The compiler generates 'stub' definition header files from IDL
specifications. It currently works for definitions written in the JMG
language (although this is still very much a work in progress), and
also has limited support for QuickFIX XML definitions (written as a
proof of concept without much support or testing for intracting with
actual FIX messages).

One experimental idea under consideration here is to have **jmgc**
support generating specificiations in other IDLs where Jormungandr has
support for wrapping their native objects. An example would be
protobuf and the idea is to have _standard interface_ wrapper support
for protobuf objects and **jmgc** support for generating .proto
files. This would allow the source of ground truth for the system
protocol to be JMG language and would be valuable even if Jormungandr
has direct implementation of serialization and deserialization of
protobuf wire format (under consideration) because protoc could still
be used to generate the bindings for other languages such as Java or
Python.

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

## High performance event processing

Not much work here yet, but the idea is to mix the _standard
interface_ style of messaging with a high performance event loop based
on fibers and io_uring on Linux. This is a big lift, and the only work
done so far is a bit of exploration and prototyping to try to find a
clean interface that is consistent with a specific internal vision of
how it should look from the user perspective. I may try to create
something using existing boost tools that will produce the desired
user-facing style and then work to steadily swap out the internals for
improved performance.

# Coding standards

A corollary to the philosophy of expecting that users will be familiar
with the code base is the requirement to make the familiarization
process as easy as possible through documentation. This section
contains a documentation of some of the standards associated with the
project, especially those that may differ from expected practice.

In general, code should follow the C++ core guidelines, with a few
exceptions and additions as outlined here

## General principles

These may or may not be controversial but they have served me well.

* If it can be **const** then it should be **const**
* Impute semantic meaning to types using the _safe_ types framework
* Use metaprogramming to make interfaces more robust
* **Always** use exceptions, and rely on RAII for error handling

## Standard abbreviations

Naming is well known to be one of the hard problems in software
engineering. To facilitate the appropriate naming of things while
keeping the code compact, a series of standard abbreviations is
catalogued here. If you see an abbreviation whose meaning is not
obvious, look in this section for a defintion. When contributing code,
please use these abbreviations where appropriate.

* cncy - currency
* ctrl - control/controller
* def  - definition
* fbr  - fiber
* fld  - field
* msg  - message
* px   - price
* req  - request
* rslt - result, typically used to name an automatic variable that
  will return a value from a function so as to take advantage of named
  return value optimization
* rsp  - response
* srvr - server
* str  - string
* strm - stream
* svc  - service
* sz   - size/length/count
* tp   - time point
* ts   - timestamp

## Some other naming standards

### Common prefixes

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
