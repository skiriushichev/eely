#include "example_state_machine/app_example_state_machine.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_state_machine app{1024, 768, "Example: state machine"};
  return app.run();
}