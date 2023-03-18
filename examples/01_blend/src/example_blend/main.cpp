#include "example_blend/app_example_blend.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_blend app{1024, 768, "Example: blend"};
  return app.run();
}