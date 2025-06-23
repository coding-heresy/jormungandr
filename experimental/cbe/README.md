
# Summary

Experimental encoding format that attempts to mix Google protobuf
varint with FIX-FAST generic stop bit encoding and add in support for
compressing floating point numbers. Difficult to say whether the
floating point compression will have value because it mostly depends
on how much precision is being stored in any given application,
although having a lot of zeros will compress well. This will also
serve as a test bed for some features of interest:
* Supporting separate object-based and stream-based decoding
  algorithms.
* Using non-owning objects wherever appropriate/possible.
* Finding a way to integrate polymorphic memory allocators.
* Handling strings of characters and sequences of other types
  (i.e. arrays, vectors and `span`) with the same code where
  possible. Perhaps this will involve creating concepts that create a
  proper hierarchy for objects that own memory as compared with those
  that view or proxy memory.

# TODO list

* Add CBE/native support to jmgc.
* Continue developing support for 'viewable' types
  * Improve 'viewable' type handling to support 'pmr'
  * Export 'viewable' type handling to other encodings
* Add exhaustive tests of cases where buffers are too small to encode
  or have been truncated before decode
* Add some metaprogramming to constrain fields to contain only
  allowable types, especially which types are allowed to be in arrays
* Rethink and rework buffer handling. The current implementation only
  works with fixed-size buffers represented using std::span, which
  will complicate things when encoding with sub-objects (do to the
  need to prefix the length) and will prevent use of multiple buffers
  where objects cross buffer boundaries. Consider a concept that
  combines `std::ranges::range` with requirements for `size()` and
  stateful `next()` member functions. `absl::cord` could form the
  basis for an initial implementation.
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
* Rethink floating point compaction: the current implementation isn't
  very effective (i.e. the default encoding of 42.0 requires 10
  octets) and this seems to indicate that perhaps specifically
  encoding the mantissa in chunks from highest to lowest will produce
  much better results than simply treating it as an integer. It also
  makes sense to have a compiler flag that disables floating point
  compression.
  * More thoughts on this: reversing the direction of scanning
    mantissa octets does seem to make sense based on experimental
    evidence, although more samples should be viewed
  * There are extra bits available in the exponents to allow the
    addition of another metadata bit that would indicate whether or
    not the mantissa was compressed: if the bit is set, the mantissa
    is compressed with stop-bit encoding and should require fewer
    octets than the native encoding; otherwise, the mantissa octets
    should be copied without change.
