module app;

import std;

import subprocess;
import argh;
import nlohmann.json;

namespace sp = subprocess;
using json = nlohmann::json;

void app(argh::parser args){
  std::println("Hello world!");
  if(args[{"-v", "--verbose"}]){
    std::println("verbose messaging");
  }
}


