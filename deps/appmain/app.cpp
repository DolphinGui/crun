module;

// main can't be a part of a module,
// but we define it inside of this
// dependency so it's declared here
int main(int argc, char* argv[]);

export module app;

export import argh;

export void app(argh::parser);

