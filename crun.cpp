import std;

import subprocess;
import argh;
import nlohmann.json;

namespace sp = subprocess;
using json = nlohmann::json;

int main(int argc, char* argv[]){
  std::println("hello world!");
  auto obuf = sp::check_output({"lsd", "-l"});
  std::println("files: {}", obuf.buf.data());
  argh::parser cmdl(argv);

  if (cmdl[{ "-v", "--verbose" }])
    std::println("I am verbose");
  json data = json::parse(R"(["hi", "hello"])");
  std::println("array: {}", data.dump());
}


