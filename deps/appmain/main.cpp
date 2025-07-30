import app;
import std;

int main(int argc, char* argv[]){
  try{
    auto params = argh::parser(argv);
    app(std::move(params));
  }catch (std::runtime_error const& e){
    std::println("Runtime error caught: {}", e.what());
  }catch(...){

  }
}
