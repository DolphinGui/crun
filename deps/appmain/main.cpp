import app;
import std;

int main(int argc, char *argv[]) {
  try {
    auto args = cli_parser.parse(argc, argv);
    app(std::move(args));
  } catch (std::runtime_error const &e) {
    std::println("Runtime error caught: {}", e.what());
    return 1;
  } catch (...) {
    return 2;
  }
}
