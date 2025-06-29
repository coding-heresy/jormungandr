
# Reactor high performance server framework

Here's an excellent example of tilting at windmills. I would like to
have a server framework with the following characteristics:
* I/O should be fully asynchronous, based on io_uring.
* The programming style should appear to be fully synchronous. In
  particular, there should be no whacky `promise`/`future` chaining
  like in Seastar, nor should there be piles of callback spaghetti as
  in boost.asio. Domain code should have the appearance of
  straight-line handlers even as they are executing RPCs that require
  waiting for responses.
* To smooth the programming experience, the gaps created by
  asynchronous communication should be papered by using fibers (AKA
  _green_/_user space_/_cooperating_ threads). This would require
  rigorous discipline from engineers to carefully understand object
  lifetimes and which thread a given piece of code would execute on,
  which I'm fine with.
* The preferred API would be influenced by boost.asio and its
  forebears: work originating from outside the main I/O thread can be
  _posted_ to it for execution, and CPU-heavy processing can be
  _posted_ out to thread pools to avoid delaying I/O operations.
* Most particularly, I have no current desire to work with anything
  involving the _executors_ proposal(s) because reading those papers
  always gives me a headache along with the distinct impression that
  either 1) they are solving a problem that I don't really have or 2)
  I'm an idiot.
With all of that said, I have been sadly disappointed with the quality
of the various fiber implementations floating around out there
(e.g. boost and folly at least). There seems to be a lot of
hand-wringing about the risks and difficulties of using them and a lot
of extraneous complexity. What I want is something that runs
'parallel' to the existing _thread_ interfaces and has tightly
coupled, 'colorful' (in the sense of 'what color is your function')
interfaces and functions that are similar to existing interfaces and
functions. Here I'm imagining fiber-specific versions of e.g. `read`,
`write`, `listen`, `accept`, `promise`, `future`, etc that can then be
used to build up equivalent fiber-specific implementations of
e.g. HTTP servers and clients. The (likely vain) hope is that once the
basics are in place, a lot of the higher-level implementations can be
liberally cribbed from existing open source codebases.

Anyway, with that said, I've started to dig in to building the fiber
foundations for a class called `Reactor` that is intended to control a
single io_uring event loop. Within `Reactor`, the programming style is
expected to be informed by traditional operating systems constructs to
drive better performance. This is going slowly, with some earlier
false starts that foundered over the poor documentation and
bewildering design choices of the System V context control
API. However, I was finally able to grasp a couple of subtle points
and prune away some of the more bizarre possible behaviors to arrive
at a design that I can now attempt to carefully mesh with the io_uring
API to produce something useful.

# TODO

* Complete the first test case: start the reactor and shut it down
  without having it explode violently.
* ...
* Profit!
