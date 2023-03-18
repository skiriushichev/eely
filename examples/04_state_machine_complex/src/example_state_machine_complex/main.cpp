#include "example_state_machine_complex/app_example_state_machine_complex.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_state_machine_complex app{1024, 768, "Example: state machine (complex)"};
  return app.run();
}