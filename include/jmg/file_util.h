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

#include <iostream>
#include <filesystem>
#include <fstream>

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// useful concepts
////////////////////////////////////////////////////////////////////////////////
namespace detail
{
template<typename T>
concept IoStreamT = std::same_as<T, std::ifstream>
  || std::same_as<T, std::ofstream>;

// TODO move to metaprogramming utility header?
template<typename T>
concept StringT = std::convertible_to<T, std::string_view>;
} // namespace detail

/**
 * Open an input or output stream for a file
 *
 * @param path filesystem path to the file, can be any type supported
 *        by the std::filesystem::path constructor
 */
template <detail::IoStreamT StreamT, detail::StringT PathNameT>
auto openFile(PathNameT pathName) {
  StreamT strm;
  strm.exceptions(StreamT::badbit);
  // convert to filesystem path for a bit of extra safety checking
  std::filesystem::path filePath{pathName};
  strm.open(filePath);
  return strm;
}

} // namespace jmg
