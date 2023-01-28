#include "example_blend/app_example_blend.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_blend app{1280, 1024, "Example: blendtree"};
  return app.run();
}