#include "example_btree/app_example_btree.h"

#include <SDL.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_example_btree app{1024, 768, "Example: blendtree"};
  return app.run();
}