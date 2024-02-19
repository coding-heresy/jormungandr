/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include <ext/stdio_filebuf.h>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// useful concepts
////////////////////////////////////////////////////////////////////////////////
namespace detail
{
template<typename T>
concept IoStreamT =
  std::same_as<T, std::ifstream> || std::same_as<T, std::ofstream>;
} // namespace detail

/**
 * open an input or output stream for a file
 *
 * @param filePath filesystem path to the file
 */
template<detail::IoStreamT Strm>
auto open_file(const std::filesystem::path filePath) {
  Strm strm;
  // throw exception on both badbit and failbit to force open() to
  // throw on failure
  strm.exceptions(Strm::badbit | Strm::failbit);
  strm.open(filePath);
  // clear failbit from exception bits to avoid having an exception
  // thrown when the stream hits EOF
  strm.exceptions(Strm::badbit);
  return strm;
}

/**
 * open an input or output stream for a file
 *
 * @param pathName filesystem path to the file, can be any type
 *        supported by the std::filesystem::path constructor
 */
template<detail::IoStreamT Strm, StringLikeT PathNameT>
auto open_file(PathNameT pathName) {
  return open_file<Strm>(std::filesystem::path(pathName));
}

/**
 * class manages a temporary file, opening and writing data to it on
 * instantiation, providing its name to clients and removing it from
 * the filesystem when the instance is destroyed
 */
class TmpFile {
public:
  /**
   * constructor
   *
   * @param data to write to the file after creation
   */
  explicit TmpFile(const std::string_view contents) {
    char fileName[1024];
    memset(fileName, 0, sizeof(fileName));
    {
      const auto tmpPath = std::filesystem::temp_directory_path() /= "XXXXXX";
      JMG_ENFORCE(tmpPath.native().size() < 1024,
                  "unable to create temporary file, intended file base name ["
                    << tmpPath.native()
                    << "] was longer than internal limit value [1024]");
      tmpPath.native().copy(fileName, tmpPath.native().size());
    }
    const int fd = mkstemp(fileName);
    if (-1 == fd) { JMG_THROW_SYSTEM_ERROR("unable to create temporary file"); }
    auto tmpBuf = __gnu_cxx::stdio_filebuf<char>(fd, std::ios::out);
    auto strm = std::ostream(&tmpBuf);
    strm.exceptions(std::ofstream::badbit);
    strm << contents;
    path_ = fileName;
    native_ = path_.native();
  }

  std::string_view name() const { return native_; }

  ~TmpFile() { std::filesystem::remove(path_); }

private:
  std::filesystem::path path_;
  std::string native_;
};

} // namespace jmg
