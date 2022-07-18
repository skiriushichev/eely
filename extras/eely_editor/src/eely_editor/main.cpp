#include <SDL.h>

#include <eely_editor/app_editor.h>

int main(int /*argc*/, char** /*argv*/)
{
  using namespace eely;

  app_editor app{1024, 768, "Eely Editor"};
  return app.run();
}