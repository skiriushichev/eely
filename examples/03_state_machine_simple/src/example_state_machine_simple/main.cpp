#include "example_state_machine_simple/app_example_state_machine_simple.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_state_machine_simple app{1024, 768, "Example: state machine (simple)"};
  return app.run();
}