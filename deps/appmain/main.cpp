import app;
import std;

int main(int argc, char *argv[]) {
  try {
    app(cli_parser.parse(argc, argv));
  } catch (std::runtime_error const &e) {
    std::println("Runtime error caught: {}", e.what());
    return 1;
  } catch (...) {
    return 2;
  }
}
