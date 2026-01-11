
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

Anyway, I've started to dig in to building the fiber foundations for a
class called `Reactor` that is intended to control a single io_uring
event loop. Within `Reactor`, the programming style is expected to be
informed by traditional operating systems constructs to drive better
performance. This is going slowly, with some earlier false starts that
foundered over the poor documentation and bewildering design choices
of the System V context control API. However, I was finally able to
grasp a couple of subtle points and prune away some of the more
bizarre possible behaviors to arrive at a design that I can now
attempt to carefully mesh with the io_uring API to produce something
useful.

## Important design milestone

I've always wanted a way to confine the operations performed within a
fiber to those which are appropriate to the cooperative multitasking
environment without being too intrusive about it. While it is
basically impossible to absolutely forbid blocking operations
(including logging via the standard streaming I/O operators), I have
found a way to provide easy access to the restricted set of
operations: I will code all of them as member functions of the `Fiber`
class, and then have all callback functions (typically provided by the
user as lambdas) take a non-`const` reference to the `Fiber` object
associated with its logical _fiber_ of control. Within the callback,
required operations within the reactor (such as reading/writing data,
opening files, etc) will be accessed via the `Fiber`. An example of
what low-level code might look like would be the following:
```
using ListenerFcn = std::function<void(Fiber&, Socket)>;
//...
Reactor reactor;

// read configuration for the listener, such as the port number to
// listen to, from some configuration source
const auto listener_cfg = ListenerCfg(cfg);

// start the reactor with a listener fiber that calls its callback in
// a new fiber whenever a new connection is accepted

reactor.start(listener_cfg, [](Fiber& fbr, Socket socket) {
  auto msg_buf = Buffer<kMsgSz>;
  fbr.read(socket, msg_buf);
  const auto req = deserializeReq(msg_buf);
  const auto rsp_buf = serializeRsp(processReq(req));
  fbr.write(socket, rsp_buf);
});

```

This feels like a good starting point, and could be used to construct
higher-level abstractions.

# Abstractions for distributed interactions

## RPC

### Server
```
// receive requests from clients, passing each one to a thread pool
// for processing and sending the result back to the requesting
// client

auto worker_pool = ThreadPool(app_cfg);
auto reactor = Reactor(worker_pool);
auto svc_cfg = SvcCfg(app_cfg);
reactor.start(svc_cfg, [](Fiber& fbr, const RpcReq& req) {
  // fiber sends the work to worker_pool and is blocked while the work
  // is happening
  auto rsp = fbr.executeRemote([&] {
    return processReq(req);
  });
  req.respondWith(rsp);
});
```

### Client
```
// read a file of requests, sending each request to a compute server
// and storing the compute responsess to a database

auto compute_svc_cfg = ComputeSvcCfg(app_cfg);
auto db_svc_cfg = DbSvcCfg(app_cfg);
auto data_file_path = jmg::get<DataFilePathFld>(app_cfg);
Reactor reactor;
reactor.start([&](Fiber& fbr) {
  auto compute_svc = ComputeSvc(fbr, compute_svc_cfg);
  auto db_svc = DbSvc(fbr, db_svc_cfg);
  auto fd = fbr.open(data_file_path);
  auto deserialize_view = Deserializer();
  for (const auto& req : LineReader(fd) | deserialize_view) {
    // compute_svc returns a stream of multiple responses for each
    // request
    for (const auto& compute_rsp : compute_svc.rpc(req)) {
      JMG_ENFORCE(pred(compute_rsp.isOk()), "compute request failed: [",
                  compute_rsp.error(), "]");
      const auto db_rsp = db_svc.rpc(makeDbUpdateFrom(rsp));
      JMG_ENFORCE(pred(db_rsp.isOk()), "DB update failed: [",
                  db_rsp.error(), "]");
    }
  }
});
```

## Publish/Subscribe

### Server
```
auto broker_cfg = BrokerCfg(app_cfg);
auto_svc_cfg = SvcCfg(app_cfg);
reactor.start([&](Fiber& fbr) {
  auto broker_svc = BrokerSvc(fbr, broker_cfg);
  auto topic1 = broker_svc.topicCnxn(jmg::get<Topic1Fld>(app_cfg));
  auto topic2 = broker_svc.topicCnxn(jmg::get<Topic2Fld>(app_cfg));
  // RPC listener takes over the fiber
  fbr.listen(svc_cfg, [&](Fiber& fbr, const RpcReq& req) {
    if (isForTopic1(req)) {
      topic1.publish(getTopic1Data(req));
    }
    else {
      JMG_ENFORCE(isForTopic2(req), "unknown topic destination for request");
      topic2.publish(getTopic2Data(req));
    }
  });
});

```

# Design notes

## `yield` function (stale, see scheduling algorithm below)
* ~~if the _runnable_ list is not empty~~
  * ~~grab the first element from the _runnable_ list and make it _next active_~~
  * ~~store the current fiber context in its FCB~~
  * ~~add the current fiber to the tail of the _runnable_ list~~
  * ~~jump to the _next active_ fiber~~
* ~~if the _runnable_ list is empty~~
  * ~~poll the ring for the list of new events~~
  * ~~if there are no new events, return to executing the yielded fiber~~
  * ~~if there are new events, process them and restart the scheduling~~

Should probably just set a flag marking the `current` fiber as
`yielding`:

* mark the `current` fiber as `yielding`
* execute the scheduling algorithm

## _request_ function

* submit the request to the ring
* execute the scheduling algorithm

## scheduling algorithm
Scheduler has 2 states: `polling` and `blocking`

* mark the `state` as `polling`
* `while (true)`
  * if the `runnable` queue is not empty
    * dequeue the first element as `next active`
    * if the `active` fiber is yielding
      * enqueue the `active` fiber in the `runnable` queue
      * clear the flag that indicates whether the `active` fiber is yielding
    * if the `active` fiber ID matches the ID of the current fiber
      * return from the scheduler (`active` fiber is resuming)
    * store the current fiber context in its FCB
      * execution will jump back to this point
      * if the `active` fiber ID does not match the ID of the current fiber
        * set the `active` fiber ID to the ID of the current fiber
        * return from the scheduler function
      * else (`active` fiber ID matches the ID of the current fiber, jumping away)
        * set the `active` fiber ID to the ID
        * jump to the `next active` fiber (does not return)
  * else (`runnable` queue is empty)
    * if `state` is `polling`
      * mark the `state` as `blocking`
      * poll the ring for the list of new events
    * else
      * wait for the ring to deliver new events
    * process any events that have occurred
      * handle the special case of loop shutdown request
      * handle the special case of incoming work request
      * handle completion events by adding the related fiber to the `runnable` queue
        * also need to store the event in the FCB of the related thread
        * NOTE: handling of the CQE result is the responsibility of the related fiber

# TODO

* Review and rework fiber state handling
  * There are currently some weird flags floating around that should
    be folded into the set of states
* Maybe create an async retread of the various `std::filesystem`
  functions
* Investigate buffer registration
* Investigate direct descriptors
* Figure out how to use c-ares for DNS resolution

## Longer term
* Try to come up with a better way to pass a fiber worker function
  between threads. The current implementation requires that it be
  allocated on the heap and passed as a pointer, which isn't the best
* Should the implementation of type-erased functions be switched from
  `std::function` to a moveable function type?
