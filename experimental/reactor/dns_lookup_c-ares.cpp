/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Brian Davis <brian8702@sbcglobal.net>
 *
 */

#include "dns_lookup.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <tuple>

// #include "jmg/util.h"

using namespace std;

namespace jmg
{

namespace
{
template<typename T>
  requires IntegralT<T> || EnumT<T>
void checkAresStatus(const T status) {
#if 1
  if (ARES_SUCCESS == status) { return; }
  throw runtime_error(str_cat("DNS lookup failure: ",
                              ares_strerror(static_cast<int>(status))));
#else
  if (ARES_SUCCESS == status) { return; }
  switch (status) {
    case ARES_ENODATA:
      throw runtime_error("no relevant answer for DNS lookup");
    case ARES_EFORMERR:
      throw runtime_error(
        "internal error: invalid argument at some stage of  DNS lookup");
    case ARES_EFILE:
      throw runtime_error("unable to find required config file for DNS lookup");
    case ARES_ENOMEM:
      throw bad_alloc();
    case ARES_ENOTINITIALIZED:
      // should not happen, by construction
      throw logic_error(
        "DNS lookup attempted before library was properly initialized");
    case ARES_ENOSERVER:
      throw runtime_error("no DNS servers available to perform lookup");

      // TODO(bd) handle the rest

    default:
      throw logic_error(str_cat("ares_init_options returned unknown value [",
                                status, "]"));
  }
#endif
}

/**
 * helper function that converts user data pointer from c-ares VTable
 * to reference to Fiber
 */
Fiber& getFiberRefForVtable(void* user_data) {
  // NOTE: this should unwind through the C stack of the ares lookup,
  // possibly breaking things, but seems like the only recourse other
  // than `abort` at this point...
  JMG_ENFORCE_USING(logic_error, user_data,
                    "no fiber ref provided for c-ares vtable entry");
  return *(reinterpret_cast<Fiber*>(user_data));
}

tuple<int, int> xlateAresSockOpt(const ::ares_socket_opt_t opt) {
  switch (opt) {
    case ARES_SOCKET_OPT_SENDBUF_SIZE:
      return make_tuple<int, int>(SOL_SOCKET, SO_SNDBUF);
    case ARES_SOCKET_OPT_RECVBUF_SIZE:
      return make_tuple<int, int>(SOL_SOCKET, SO_RCVBUF);
    case ARES_SOCKET_OPT_BIND_DEVICE:
      return make_tuple<int, int>(SOL_SOCKET, SO_BINDTODEVICE);
    case ARES_SOCKET_OPT_TCP_FASTOPEN:
      return make_tuple<int, int>(SOL_TCP, TCP_FASTOPEN);
    default:
      break;
  }
  throw runtime_error(str_cat("received unknown c-ares socket option type [",
                              static_cast<int>(opt), "]"));
}

#define JMG_ARES_VTABLE_SINK_EXCEPTIONS()   \
  catch (const system_error& e) {           \
    errno = e.code().value();               \
  }                                         \
  catch (...) {                             \
    /* seems like a reasonable catch-all */ \
    errno = EIO;                            \
  }                                         \
  return -1

/**
 * c-ares VTable function that opens a socket via a provided fiber
 */
::ares_socket_t openAresSocket(const int domain,
                               const int sock_type,
                               const int /*protocol*/,
                               void* user_data) {
  auto& fbr = getFiberRefForVtable(user_data);
  switch (domain) {
    case AF_INET:
      break;
      // TODO(bd) also support AF_INET6
    default:
      return ARES_SOCKET_BAD;
  }

  SocketTypes socket_type = SocketTypes::kTcp;
  switch (sock_type) {
    case SOCK_STREAM:
      // default value was already set
      break;
    case SOCK_DGRAM:
      socket_type = SocketTypes::kUdp;
      break;
    default:
      return ARES_SOCKET_BAD;
  }

  try {
    fbr.log("!!!!! opening socket");
    auto dbg_log = Cleanup([&] { fbr.log("!!!!! done opening socket"); });
    return static_cast<::ares_socket_t>(unsafe(fbr.openSocket(socket_type)));
  }
  catch (const system_error& e) {
    errno = e.code().value();
  }
  catch (...) {
    // seems like a reasonable catch-all
    errno = EIO;
  }
  return ARES_SOCKET_BAD;
}

int closeAresSocket(::ares_socket_t sock, void* user_data) {
  auto& fbr = getFiberRefForVtable(user_data);
  try {
    fbr.log("!!!!! closing socket");
    fbr.close(SocketDescriptor(static_cast<int>(sock)));
    fbr.log("!!!!! done closing socket");
    return 0;
  }
  JMG_ARES_VTABLE_SINK_EXCEPTIONS();
}

int setAresSockOpts(ares_socket_t sd,
                    ares_socket_opt_t ares_opt,
                    const void* opt_val,
                    ares_socklen_t opt_sz,
                    void* user_data) {
  auto& fbr = getFiberRefForVtable(user_data);
  try {
    const auto [level, sys_opt] = xlateAresSockOpt(ares_opt);
    JMG_ENFORCE(opt_sz > 0, "non-position option size [", opt_sz,
                "] provided by c-ares when setting socket option");
    fbr.log("!!!!! setting socket options");
    fbr.setSocketOption(SocketDescriptor(sd), level, sys_opt, opt_val,
                        static_cast<size_t>(opt_sz));
    fbr.log("!!!!! done setting socket options");
    return 0;
  }
  JMG_ARES_VTABLE_SINK_EXCEPTIONS();
}

int connectAresSocket(::ares_socket_t sock,
                      const struct sockaddr* addr,
                      ares_socklen_t addr_len,
                      // TODO(bd) handle flags?
                      unsigned int /* flags */,
                      void* user_data) {
  JMG_ENFORCE(addr, "no address provided for c-ares socket connection");
  JMG_ENFORCE(addr_len > 0, "non-positive value [",
              static_cast<int64_t>(addr_len),
              "] provided for c-ares address structure size");
  auto& fbr = getFiberRefForVtable(user_data);
  try {
    const sockaddr_in& dbg_addr_in = *((const sockaddr_in*)addr);
    string dbg_endpoint = from(dbg_addr_in);
    fbr.log("!!!!! connecting socket to endpoint [", dbg_endpoint, "]");
    fbr.connectTo(SocketDescriptor(static_cast<int>(sock)),
                  IpEndpoint(*addr, static_cast<size_t>(addr_len)));
    fbr.log("!!!!! done connecting socket");
    return 0;
  }
  JMG_ARES_VTABLE_SINK_EXCEPTIONS();
}

::ares_ssize_t recvFromAresSocket(ares_socket_t sd,
                                  void* buf,
                                  size_t sz,
                                  int flags,
                                  struct sockaddr* /* address */,
                                  ares_socklen_t* /* address_len */,
                                  void* user_data) {
  auto& fbr = getFiberRefForVtable(user_data);
  auto buf_proxy = BufferProxy(reinterpret_cast<uint8_t*>(buf), sz);
  try {
    fbr.log("!!!!! receiving from socket");
    auto dbg_log =
      Cleanup([&] { fbr.log("!!!!! done receiving from socket"); });
    return static_cast<::ares_ssize_t>(fbr.recvFrom(SocketDescriptor(sd),
                                                    buf_proxy, flags));
  }
  JMG_ARES_VTABLE_SINK_EXCEPTIONS();
}

::ares_ssize_t sendToAresSocket(::ares_socket_t sd,
                                const void* buf,
                                size_t sz,
                                int /* flags */,
                                const struct sockaddr* /* address */,
                                ares_socklen_t /* address_len */,
                                void* user_data) {
  auto& fbr = getFiberRefForVtable(user_data);
  auto buf_view = BufferView(reinterpret_cast<const uint8_t*>(buf), sz);
  try {
    fbr.log("!!!!! sending to socket");
    auto dbg_log = Cleanup([&] { fbr.log("!!!!! done sending to socket"); });
    return static_cast<::ares_ssize_t>(fbr.write(SocketDescriptor(sd),
                                                 buf_view));
  }
  JMG_ARES_VTABLE_SINK_EXCEPTIONS();
}

#undef JMG_ARES_VTABLE_SINK_EXCEPTIONS

} // namespace

DnsLookup& DnsLookup::instance() {
  static auto instance = DnsLookup();
  return instance;
}

DnsLookup::DnsLookup() { ::ares_library_init(ARES_LIB_INIT_ALL); }

DnsLookup::~DnsLookup() { ares_library_cleanup(); }

string DnsLookup::lookup(Fiber& fbr,
                         c_string_view host,
                         DnsLookup::OptTimeout timeout) {
  const auto vtable = makeSocketFcns();

  // NOTE: using automatic variables + std::tie here to ensure correct
  // lifetime
  int mask;
  LookupOpts opts;
  tie(mask, opts) = makeLookupOpts(timeout);

  Channel* channel = nullptr;
  checkAresStatus(::ares_init_options(&channel, &opts, mask));
  checkAresStatus(::ares_set_socket_functions_ex(channel, &vtable, &fbr));
  struct ::ares_addrinfo_hints hints = {
    .ai_flags = 0,
    // TODO(bd) support IPv6
    .ai_family = AF_INET,
    // TODO(bd) support UDP
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
  };

  LookupResult lookup_rslt;
  ::ares_getaddrinfo(channel, host.c_str(), nullptr /* service */, &hints,
                     DnsLookup::aresCallback, &lookup_rslt);
  if (lookup_rslt.exc_ptr) { rethrow_exception(lookup_rslt.exc_ptr); }
  return lookup_rslt.addr;
}

tuple<int, DnsLookup::LookupOpts> DnsLookup::makeLookupOpts(OptTimeout timeout) {
  int mask = 0;
  auto opts = LookupOpts{};
  // TODO(bd) relax this limitation to using only TCP once the
  // reactor supports UDP
  opts.flags = ARES_FLAG_USEVC;
  if (timeout) {
    mask |= ARES_OPT_TIMEOUTMS;
    opts.timeout = timeout->count();
  }
  return make_tuple(mask, opts);
}

DnsLookup::VTable DnsLookup::makeSocketFcns() {
  VTable rslt;
  memset(&rslt, 0, sizeof(rslt));
  rslt.version = 1;
  rslt.asocket = openAresSocket;
  rslt.aclose = closeAresSocket;
  rslt.asetsockopt = setAresSockOpts;
  rslt.aconnect = connectAresSocket;
  rslt.arecvfrom = recvFromAresSocket;
  rslt.asendto = sendToAresSocket;
  return rslt;
}

void DnsLookup::aresCallback(void* user_data,
                             int status,
                             int /*timeouts*/,
                             DnsLookup::AresAddrInfo* raw_result) {
  cout << ">>>>> starting aresCallback" << endl;
  const auto deallocate = Cleanup([&]() {
    if (raw_result) { ares_freeaddrinfo(raw_result); }
  });
  // NOTE: this should unwind through the C stack of the ares lookup,
  // possibly breaking things, but seems like the only recourse other
  // than `abort` at this point...
  JMG_ENFORCE_USING(logic_error, user_data,
                    "no lookup result ref provided for c-ares callback");
  auto& rslt = *(reinterpret_cast<LookupResult*>(user_data));
  try {
    checkAresStatus(status);
    JMG_ENFORCE(raw_result, "c-ares lookup status indicated success but no raw "
                            "result list was available");
    for (auto node = raw_result->nodes; node; node = node->ai_next) {
      cout << ">>>>> checking result node" << endl;
      void* ptr = nullptr;
      switch (node->ai_family) {
        case AF_INET:
          cout << ">>>>> found IPv4 record" << endl;
          ptr = (void*)(&((struct sockaddr_in*)node->ai_addr)->sin_addr);
          break;
        // TODO(bd) support AF_INET6 as well
        // case AF_INET6:
        //   ptr = (void*)(&((struct sockaddr_in*)node->ai_addr)->sin6_addr);
        //   break;
        default:
          // unknown address family, try the next one
          cout << ">>>>> skipping record for unknown protocol" << endl;
          continue;
      }
      if (ptr) {
        char buf[INET6_ADDRSTRLEN + 1];
        memset(buf, 0, sizeof(buf));
        cout << ">>>>> calling ares_inet_ntop to populate buffer of size ["
             << sizeof(buf) << "]" << endl;
        ::ares_inet_ntop(node->ai_family, ptr, buf, sizeof(buf));
        cout << ">>>>> resulting address was of length [" << strlen(buf) << "]"
             << endl;
        rslt.addr.reserve(INET6_ADDRSTRLEN);
        rslt.addr = buf;
        // return the first result
        return;
      }
    }
    throw runtime_error("c-ares lookup status indicated success but no "
                        "acceptable addresses were found");
  }
  catch (...) {
    cout << ">>>>> caught exception" << endl;
    rslt.addr.clear();
    rslt.exc_ptr = current_exception();
  }
}

} // namespace jmg
