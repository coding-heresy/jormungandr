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

#include <filesystem>

#include "examples/monster_generated.h"
#include "flatbuffers/flatbuffers.h"

#include "jmg/bitwise_scoped_enums.h"
#include "jmg/cmdline.h"
#include "jmg/reactor/reactor_based_client.h"

using namespace std;
using namespace std::string_literals;

namespace jmg
{

class FlatbufMonstersDemo : public ReactorBasedClient {
  using ReadFlag = NamedFlag<
    "read",
    "set flag to indicate that the file should be read instead of written">;
  using FileName = PosnParam<string, "filename", "path to file">;
  using CmdLine = CmdLineArgs<ReadFlag, FileName>;

public:
  FlatbufMonstersDemo() = default;
  virtual ~FlatbufMonstersDemo() = default;

  void processArguments(const int argc, const char** argv) override {
    const auto cmdline = CmdLine(argc, argv);
    read_flag_ = get<ReadFlag>(cmdline);
    file_name_ = get<FileName>(cmdline);
  }

  void execute(Reactor& reactor) override {
    auto [completion_signal, completion_rcvr] = makeSignaller();
    reactor.execute([&](Fiber& fbr) {
      try {
        if (read_flag_) { readData(fbr); }
        else { writeData(fbr); }
        completion_signal.set_value();
      }
      catch (...) {
        completion_signal.set_exception(current_exception());
      }
    });

    completion_rcvr.get(2s);
    cout << "demo complete\n";
  }

private:
  void readData(Fiber& fbr) {
    const auto file_path = filesystem::path(file_name_);
    JMG_ENFORCE(filesystem::is_regular_file(file_path), "file [", file_name_,
                "] is not a regular file");
    const auto data_sz = filesystem::file_size(file_path);
    const auto fd = fbr.openFile(file_path, FileOpenFlags::kRead);
    string buf;
    buf.resize(data_sz);
    {
      const auto sz = fbr.read(fd, buffer_from(buf));
      JMG_ENFORCE(sz == data_sz, "failed reading data from file, read [", sz,
                  "] but expected [", data_sz, "]");
    }
    auto monster = MyGame::GetMonster(buf.data());
    std::cout << "Monster Name: " << monster->name()->c_str() << std::endl;
    std::cout << "Monster HP:   " << monster->hp() << std::endl;
    auto inventory = monster->inventory();
    if (inventory) {
      std::cout << "Inventory size: " << inventory->size() << std::endl;
      for (auto item : *inventory) {
        std::cout << " - Item ID: " << static_cast<int>(item) << std::endl;
      }
    }
  }

  void writeData(Fiber& fbr) {
    flatbuffers::FlatBufferBuilder builder(1024);

    // Create a string and a vector
    auto name = builder.CreateString("Orc");
    auto inventory = builder.CreateVector(std::vector<uint8_t>{1, 2, 3});

    // Use the generated 'CreateMonster' helper
    auto orc = MyGame::CreateMonster(builder, name, 80, inventory);

    // Finish the buffer
    builder.Finish(orc);

    // Get the pointer and size for storage/transmission
    uint8_t* buf = builder.GetBufferPointer();
    const auto buf_sz = builder.GetSize();

    const auto fd = fbr.openFile(file_name_,
                                 FileOpenFlags::kWrite | FileOpenFlags::kCreate
                                   | FileOpenFlags::kTruncate,
                                 0644);
    const auto closer = Cleanup([&] { fbr.close(fd); });
    const auto buf_view = BufferView(buf, buf_sz);
    {
      cout << "writing [" << buf_sz << "] bytes to [" << file_name_ << "]\n";
      const auto sz = fbr.write(fd, buf_view);
      JMG_ENFORCE(sz == buf_sz, "failed writing data to file, wrote [", sz,
                  "] but expected to write [", buf_sz, "]");
    }
  }

  bool read_flag_ = false;
  string file_name_ = ""s;
};

JMG_REGISTER_CLIENT(FlatbufMonstersDemo);

} // namespace jmg
