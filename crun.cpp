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
  if(!args[1].empty()){
    std::println("first argument: {}", args[1]);
  }
}


