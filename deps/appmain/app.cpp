export module appmain;

export import clip;

/* This is a dummy library that exists to pull in main.cpp, which actually
 * contains main. The application must define and export app on their own, since
 * they have access to the Parser type. The App must define a app::CliParser variable
 * and export it. */
