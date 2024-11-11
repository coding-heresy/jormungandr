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
### _Safe_ Types Framework
TODO
### Return Value Overloading
TODO
### Simple Server Framework
TODO
### Standard Interface for Objects
TODO
### Command Line Parameter Handling
TODO

## Some longer-term goals

### _Standard Interface_ for _Objects_

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

#### Interface Definition Language

It can be difficult to express interfaces in the raw building blocks
of the _standard interface_ style, and some of the potential encodings
also use IDL compilers to generate code (e.g. protobuf), which is a
very effective technique. I envision creating and interface definition
language in YAML (no need to build a separate parser for this) that
will have a compiler to generate _standard interface_ definitions (and
possibly even definitions in other IDLs so that the JMG definition can
be the source of ground truth). This will be **jmgc**, and there has
been initial work already to parse the QuickFIX XML definitions and
generate standard interface code from those. The next step here will
be to design the YAML language for JMG and implement that, although I
might also take a detour into the SBE specification since I already
have XML parsing working well in the _standard interface_ and that
might help fill in some gaps in functionality before I sit down to
design the YAML language, best to have all of it in place in my head
before I start so that nothing important gets left out.

#### TODO

* A _standard interface_ for stream-based processing of messages that
  is consistent with the handling of object representation
  * The modivation here is the SAX approach to XML handling, as
    compared to the DOM approach, which would then be analogous to the
    existing implmementation that represents fully parsed/deserialized
    objects
* Creation and update of objects using the _standard interface_
* Support for more encodings such as protobuf and JSON

### High performance event processing

Not much work here yet, but the idea is to mix the _standard
interface_ style of messaging with a high performance event loop based
on fibers and io_uring on Linux. This is a big lift, and the only work
done so far is a bit of exploration and prototyping to try to find a
clean interface that is consistent with a specific internal vision of
how it should look from the user perspective. I may try to create
something using existing boost tools that will produce the desired
user-facing style and then work to steadily swap out the internals for
improved performance.

## Coding standards

A corollary to the philosophy of expecting that users will be familiar
with the code base is the requirement to make the familiarization
process as easy as possible through documentation. This section
contains a documentation of some of the standards associated with the
project, especially those that may differ from expected practice.

In general, code should follow the C++ core guidelines, with a few
exceptions and additions as outlined here

### General principles

These may or may not be controversial but they have served me well.

* If it can be **const** then it should be **const**
* Impute semantic meaning to types using the _safe_ types framework
* Use metaprogramming to make interfaces more robust
* **Always** use exceptions, and rely on RAII for error handling

### Standard abbreviations

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

### Some other naming standards

#### Common prefixes

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
