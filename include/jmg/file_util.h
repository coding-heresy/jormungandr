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
