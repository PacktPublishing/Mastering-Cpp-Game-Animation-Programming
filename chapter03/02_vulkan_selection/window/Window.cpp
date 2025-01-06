#include "Window.h"
#include "Logger.h"

bool Window::init(unsigned int width, unsigned int height, std::string title) {
  if (!glfwInit()) {
    Logger::log(1, "%s: glfwInit() error\n", __FUNCTION__);
    return false;
  }

  if (!glfwVulkanSupported()) {
    glfwTerminate();
    Logger::log(1, "%s error: Vulkan is not supported\n", __FUNCTION__);
    return false;
  }

  /* Vulkan needs no context */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

  if (!mWindow) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not create window\n", __FUNCTION__);
    return false;
  }

  mRenderer = std::make_unique<VkRenderer>(mWindow);

  glfwSetWindowUserPointer(mWindow, mRenderer.get());
  glfwSetWindowSizeCallback(mWindow, [](GLFWwindow *win, int width, int height) {
      auto renderer = static_cast<VkRenderer*>(glfwGetWindowUserPointer(win));
      renderer->setSize(width, height);
    }
  );

  glfwSetKeyCallback(mWindow, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
      auto renderer = static_cast<VkRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleKeyEvents(key, scancode, action, mods);
    }
  );

  glfwSetMouseButtonCallback(mWindow, [](GLFWwindow *win, int button, int action, int mods) {
      auto renderer = static_cast<VkRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMouseButtonEvents(button, action, mods);
    }
  );

  glfwSetCursorPosCallback(mWindow, [](GLFWwindow *win, double xpos, double ypos) {
      auto renderer = static_cast<VkRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMousePositionEvents(xpos, ypos);
    }
  );

  if (!mRenderer->init(width, height)) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not init Vulkan\n", __FUNCTION__);
    return false;
  }

  Logger::log(1, "%s: Window with Vulkan successfully initialized\n", __FUNCTION__);
  return true;
}

void Window::mainLoop() {
  /* force VSYNC */
  glfwSwapInterval(1);

  std::chrono::time_point<std::chrono::steady_clock> loopStartTime = std::chrono::steady_clock::now();
  std::chrono::time_point<std::chrono::steady_clock> loopEndTime = std::chrono::steady_clock::now();
  float deltaTime = 0.0f;

  while (!glfwWindowShouldClose(mWindow)) {
    if (!mRenderer->draw(deltaTime)) {
      break;
    }

    glfwPollEvents();

    /* calculate the time we needed for the current frame, feed it to the next draw() call */
    loopEndTime =  std::chrono::steady_clock::now();

    /* delta time in seconds */
    deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(loopEndTime - loopStartTime).count() / 1'000'000.0f;
    loopStartTime = loopEndTime;
  }
}

void Window::cleanup() {
  mRenderer->cleanup();

  glfwDestroyWindow(mWindow);
  glfwTerminate();
  Logger::log(1, "%s: Terminating Window\n", __FUNCTION__);
}
