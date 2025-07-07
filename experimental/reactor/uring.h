/** -*- mode: c++ -*-
 *
 * Copyright (C) 2025 Brian Davis
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
#pragma once

#include <optional>

#include <liburing.h>

#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"
#include "jmg/types.h"
#include "jmg/util.h"

namespace jmg::uring
{
namespace io_sqe
{

JMG_DEFINE_RUNTIME_EXCEPTION(UringFullException);

enum class SqeFlags : uint8_t {
  kNone = 0,
  kFixedFile = IOSQE_FIXED_FILE_BIT,
  kIoDrain = IOSQE_IO_DRAIN_BIT,
  kIoLink = IOSQE_IO_LINK_BIT,
  kIoHardLink = IOSQE_IO_HARDLINK_BIT,
  kAsync = IOSQE_ASYNC_BIT,
  kBufferSelect = IOSQE_BUFFER_SELECT_BIT,
  kSkipSuccess = IOSQE_CQE_SKIP_SUCCESS_BIT
};

/**
 * flags used to parameterize a filesystem stat request
 */
enum class StatFlags : int32_t {
  kNone = 0,
  kUseFdIfEmptyPath = AT_EMPTY_PATH,
  kNoAutomout = AT_NO_AUTOMOUNT,
  kNoFollowSymLink = AT_SYMLINK_NOFOLLOW,
  kSyncAsStat = AT_STATX_SYNC_AS_STAT,
  kNoSync = AT_STATX_DONT_SYNC
};

/**
 * masks used to parameterize a filesystem stat request
 */
enum class StatMask : uint32_t {
  // basic stats
  kType = STATX_TYPE,
  kMode = STATX_MODE,
  kLinkSz = STATX_NLINK,
  kUid = STATX_UID,
  kGid = STATX_GID,
  kAccessTs = STATX_ATIME,
  kModifyTs = STATX_MTIME,
  kStatusTs = STATX_CTIME,
  kINode = STATX_INO,
  kBytesSz = STATX_SIZE,
  kBlocksSz = STATX_BLOCKS,
  kAllBasic = STATX_BASIC_STATS,
  // advanced stats
  kCreateTs = STATX_BTIME,
  kMountId = STATX_MNT_ID,
  kAlignment = STATX_DIOALIGN
};

/**
 * flags used to parameterize an open request
 */
// for details on values for flags, c.f. https://linux.die.net/man/2/open
enum class OpenFlags : int32_t {
  // access modes
  kReadOnly = O_RDONLY,
  kWriteOnly = O_WRONLY,
  kReadWrite = O_RDWR,
  // creation flags
  kAppend = O_APPEND,
  kCreate = O_CREAT,
  kTruncate = O_TRUNC,
  kSync = O_SYNC,
  kEnableSigio = O_ASYNC,
  kCloseOnExec = O_CLOEXEC,
  kDirect = O_DIRECT,
  kDirectoryOnly = O_DIRECTORY,
  kExclusive = O_EXCL,
  kLargeFile = O_LARGEFILE,
  kNoAccessTimeUpdate = O_NOATIME,
  kNoControllingTerminal = O_NOCTTY,
  kNoFollowSymLink = O_NOFOLLOW,
  kNonBlocking = O_NONBLOCK,
  kNoDelay = O_NDELAY
};

/**
 * flags used to set modes for a filesystem open request
 */
enum class ModeFlags : mode_t {
  kNone = 0,
  // user
  kUserAll = S_IRWXU,
  kUserRead = S_IRUSR,
  kUserWrite = S_IWUSR,
  kUserExec = S_IXUSR,
  // group
  kGrpAll = S_IRWXG,
  kGrpRead = S_IRGRP,
  kGrpWrite = S_IWGRP,
  kGrpExec = S_IXGRP,
  // other
  kOtherAll = S_IRWXO,
  kOtherRead = S_IROTH,
  kOtherWrite = S_IWOTH,
  kOtherExec = S_IXOTH
};

/**
 * opcodes for io_uring requests
 *
 * @note the order of operations here should not be changed
 */
enum class OpCode : uint8_t {
  NoOp,
  ReadV,
  WriteV,
  FSync,
  ReadFix,
  WriteFix,
  PollAdd,
  PollDel,
  SyncFileRng,
  SendMsg,
  RcvMsg,
  TimeoutAdd, // TODO just Timeout?
  TimeoutDel,
  Accept,
  AsyncCxl,
  LinkTimeout,
  FAllocate,
  OpenAt, // open file at path
  Close,
  FilesUpdate,
  StatX, // file status
  Read,
  Write,
  FAdvise,
  MAdvise,
  Send,
  Rcv,
  OpenAt2, // extended version of OpenAt
  EPollCtrl,
  Splice,
  AddBuffers,
  DelBuffers,
  Tee,
  Shutdown,
  RenameAt,  // TODO Rename?
  UnlinkAt,  // TODO Unlink?
  MkDirAt,   // TODO MkDir?
  SymLinkAt, // TODO Symlink?
  LinkAt,    // TODO Link?
  MsgRing,
  FSetXAttr, // TODO ??
  SetXAttr,  // TODO ??
  FGetXAttr, // TODO ??
  GetXAttr,  // TODO ??
  Socket,
  UringCmd,
  SendZc,    // TODO ??
  SendMsgZc, // TODO ??
  ReadMultiShot,
  WaitId,
  FutexWait,
  FutexWake,
  FutexWaitV,
  FixedFdInstall,
  FTruncate,
  Bind,
  Listen
  // OpLast
};

/**
 * names that describe io_uring operations, this array is index-linked to the
 * OpCode enum
 */
const std::array kOperations{
  "no-op",
  "scatter read",
  "scatter write",
  "file sync",
  "read fix",
  "write fix",
  "add pollfd",
  "delete pollfd",
  "sync file range",
  "send message",
  "receive message",
  "add timeout",
  "delete timeout",
  "accept initiated connection",
  "async cancel",
  "link timeout",
  "file allocate",
  "file open",
  "close",
  "update registered file descriptors",
  "file stat",
  "read",
  "write",
  "pre-declare file access pattern",
  "pre-declare memory access pattern",
  "send",
  "receive",
  "file open (version 2)",
  "add epoll interest",
  "splice",
  "provide buffers",
  "remove buffers",
  "tee",
  "shutdown",
  "file rename",
  "file unlink",
  "make directory",
  "symlink file",
  "link file",
  "send message to ring",
  "set extended file attribute",
  "set extended attribute",
  "get extended file attribute",
  "get extended attribute",
  "create socket",
  "socket command",
  "zero copy send",
  "zero copy message send",
  "multi-shot read",
  "await child state change event",
  "await futex event",
  "send futex event",
  "await multiple futex events",
  "install fixed file descriptor",
  "truncate file",
  "bind",
  "await connection event (AKA listen)",
};

} // namespace io_sqe

////////////////////////////////////////////////////////////////////////////////
// safe types for uring
////////////////////////////////////////////////////////////////////////////////

#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using UringSz = SafeType<uint32_t>;
using UringFlags = SafeType<uint32_t>;
using UserData = SafeType<uint64_t>;
#else
JMG_NEW_SIMPLE_SAFE_TYPE(UringSz, uint32_t);
JMG_NEW_SIMPLE_SAFE_TYPE(UringFlags, uint32_t);
JMG_NEW_SIMPLE_SAFE_TYPE(UserData, uint64_t);
#endif

////////////////////////////////////////////////////////////////////////////////
// constants for uring
////////////////////////////////////////////////////////////////////////////////

constexpr auto kDefaultUringFlags = UringFlags(0);

/**
 * indication of whether the submission of requests to the io_uring
 * should be delayed to allow multiple operations, possibly linked, to
 * be submitted as efficiently as possible
 */
enum class DelaySubmission : bool { kDelay = true, kNoDelay = false };

/**
 * wrapper around the CQE result received from waiting for a uring event
 *
 * @note destroying the object will automatically mark the CQE as consumed,
 * unless this action has been explicitly canceled
 */
class Event {
public:
  Event() = default;
  JMG_DEFAULT_COPYABLE(Event);
  JMG_DEFAULT_MOVEABLE(Event);
  Event(io_uring* ring, io_uring_cqe* cqe);

  ~Event();

  operator bool() const { return pred(cqe_); }

  const io_uring_cqe& operator*() const {
    JMG_ENFORCE_USING(std::logic_error, pred(cqe_),
                      "attempted to retrieve CQE reference that was not set");
    return *cqe_;
  }

  // TODO(bd) add definition of operator->

  UserData getUserData() const {
    // NOTE: using operator* to avoid SIGSEGV if the CQE pointer is not set
    return UserData(cqe_->user_data);
  }

private:
  io_uring* ring_ = nullptr;
  io_uring_cqe* cqe_ = nullptr;
  bool is_cleanup_canceled_ = false;
};

/**
 * wrapper around an io_uring object that improves usability
 */
class Uring {
public:
  Uring() = delete;
  JMG_NON_COPYABLE(Uring);
  JMG_NON_MOVEABLE(Uring);
  ~Uring();

  // TODO(bd) allow user to pass flags as well?
  /**
   * @param sz - number of entries in the uring circular queue
   */
  explicit Uring(const UringSz sz);

  /**
   * external threads that have access to a separate io_uring can use this file
   * descriptor to send messages to this uring
   */
  FileDescriptor getChannel() const { return channel_; }

  /**
   * wait for the next uring event to occur, or until the timeout, if any
   */
  Event awaitEvent(std::optional<Duration> timeout);

  /**
   * call this after adding one or more requests to uring with delayed submission
   */
  void submitReq(const std::string_view req_type) {
    JMG_SYSTEM(io_uring_submit(&ring_), "unable to submit io_uring ", req_type,
               " request");
    // NOTE: intentionally ignoring the submission count to allow multiple
    // submissions to be chained
  }

  void registerEventNotifier(
    EventFd notifier,
    DelaySubmission isDelayed = DelaySubmission::kNoDelay);

  /**
   * submit a request for a timeout event to occur in the future
   *
   * @note this is separate from the timeout that can occur when waiting for the
   * uring to receive events, it is intended to allow external threads to
   * schedule timeout events directly with the kernel in order to avoid the need
   * to maintain such a schedule internally e.g. with a priority queue
   */
  void submitTimeoutReq(UserData data,
                        Duration timeout,
                        DelaySubmission isDelayed = DelaySubmission::kNoDelay);

private:
  io_uring_sqe& getNextSqe();

  std::optional<EventFd> notifier_;
  io_uring ring_;
  std::atomic<FileDescriptor> channel_ = kInvalidFileDescriptor;
};

} // namespace jmg::uring
