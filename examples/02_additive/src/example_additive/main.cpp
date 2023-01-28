#include "example_additive/app_example_additive.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_additive app{1280, 1024, "Example: additive"};
  return app.run();
}