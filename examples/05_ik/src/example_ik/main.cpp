#include "example_ik/app_example_ik.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_ik app{1024, 768, "Example: IK"};
  return app.run();
}