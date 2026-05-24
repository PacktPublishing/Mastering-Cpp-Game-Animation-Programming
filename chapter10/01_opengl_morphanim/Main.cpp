#include <string>

#include "Window.h"
#include "Logger.h"

int main(int argc, char *argv[]) {

  Window window;

  if (!window.init(1280, 720, "OpenGL Renderer - Morph Animations")) {
    Logger::log(1, "%s error: Window init error\n", __FUNCTION__);
    return -1;
  }

  window.mainLoop();

  return 0;
}
