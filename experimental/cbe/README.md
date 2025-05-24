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
  (i.e. arrays, vectors and span) with the same code where
  possible. Perhaps this will involve creating concepts that create a
  proper hierarchy for objects that own memory as compared with those
  that view or proxy memory.
