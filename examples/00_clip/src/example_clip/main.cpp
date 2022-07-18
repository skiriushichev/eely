#include "example_clip/app_example_clip.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_clip app{1024, 768, "Example: clip"};
  return app.run();
}