#include "Window.h"
#include "Tools.h"
#include "Logger.h"

bool Window::init(unsigned int width, unsigned int height, std::string title) {
  if (!glfwInit()) {
    Logger::log(1, "%s: glfwInit() error\n", __FUNCTION__);
    return false;
  }

  /* set a "hint" for the NEXT window created*/
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  mWindowTitle = title;
  mWindow = glfwCreateWindow(width, height, mWindowTitle.c_str(), nullptr, nullptr);

  if (!mWindow) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not create window\n", __FUNCTION__);
    return false;
  }

  glfwMakeContextCurrent(mWindow);

  mRenderer = std::make_unique<OGLRenderer>(mWindow);

  /* allow to set window title in renderer */
  ModelInstanceCamData& rendererModInstCamData = mRenderer->getModInstCamData();
  rendererModInstCamData.micGetWindowTitleFunction = [this]() { return getWindowTitle(); };
  rendererModInstCamData.micSetWindowTitleFunction = [this](std::string windowTitle) { setWindowTitle(windowTitle); };

  glfwSetWindowUserPointer(mWindow, mRenderer.get());
  glfwSetWindowSizeCallback(mWindow, [](GLFWwindow *win, int width, int height) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->setSize(width, height);
    }
  );

  glfwSetKeyCallback(mWindow, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleKeyEvents(key, scancode, action, mods);
    }
  );

  glfwSetMouseButtonCallback(mWindow, [](GLFWwindow *win, int button, int action, int mods) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMouseButtonEvents(button, action, mods);
    }
  );

  glfwSetCursorPosCallback(mWindow, [](GLFWwindow *win, double xpos, double ypos) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMousePositionEvents(xpos, ypos);
    }
  );

  glfwSetScrollCallback(mWindow, [](GLFWwindow *win, double xOffset, double yOffset) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->handleMouseWheelEvents(xOffset, yOffset);
    }
  );

  glfwSetWindowCloseCallback(mWindow, [](GLFWwindow *win) {
      auto renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(win));
      renderer->requestExitApplication();
    }
  );

  if (!mRenderer->init(width, height)) {
    glfwTerminate();
    Logger::log(1, "%s error: Could not init OpenGL\n", __FUNCTION__);
    return false;
  }

  /* use SDL for audio */
  if (!mAudioManager.init()) {
    Logger::log(1, "%s error: unable to init audio, skipping\n", __FUNCTION__);
  }

  if (mAudioManager.isInitialized()) {
    if (!mAudioManager.loadMusicFromFolder("assets/music", "mp3")) {
      Logger::log(1, "%s warning: not MP3 tracks found, skipping\n", __FUNCTION__);
    }
    if (!mAudioManager.loadMusicFromFolder("assets/music", "ogg")) {
      Logger::log(1, "%s warning: not OGG tracks found, skipping\n", __FUNCTION__);
    }
    if (!mAudioManager.loadWalkFootsteps("assets/sounds/Fantozzi-SandL1.wav")) {
      Logger::log(1, "%s warning: could not load walk footsteps, skipping\n", __FUNCTION__);
    }
    if (!mAudioManager.loadRunFootsteps("assets/sounds/Fantozzi-SandR3.wav")) {
      Logger::log(1, "%s warning: could not load run footsteps, skipping\n", __FUNCTION__);
    }
  }

  rendererModInstCamData.micIsAudioManagerInitializedCallbackFunction = [this]() { return mAudioManager.isInitialized(); };
  rendererModInstCamData.micPlayRandomMusicCallbackFunction = [this]() { mAudioManager.playRandomMusic(); };
  rendererModInstCamData.micStopMusicCallbackFunction = [this]() { mAudioManager.stopMusic(); };
  rendererModInstCamData.micPauseResumeMusicCallbackFunction = [this](bool pauseOrResume) { mAudioManager.pauseMusic(pauseOrResume); };
  rendererModInstCamData.micGetMusicPlayListCallbackFunction = [this]() { return mAudioManager.getPlayList(); };
  rendererModInstCamData.micIsMusicPausedCallbackFunction = [this]() { return mAudioManager.isMusicPaused(); };
  rendererModInstCamData.micIsMusicPlayingCallbackFunction = [this]() { return mAudioManager.isMusicPlaying(); };
  rendererModInstCamData.micGetMusicCurrentTrackCallbackFunction = [this]() { return mAudioManager.getCurrentTitle(); };
  rendererModInstCamData.micPlayNextMusicTrackCallbackFunction = [this]() { mAudioManager.playNextTitle(); };
  rendererModInstCamData.micPlayPrevMusicTrackCallbackFunction = [this]() { mAudioManager.playPrevTitle(); };
  rendererModInstCamData.micSetMusicVolumeCallbackFunction = [this](int volume) { mAudioManager.setMusicVolume(volume); };
  rendererModInstCamData.micGetMusicVolumeCallbackFunction = [this]() { return mAudioManager.getMusicVolume(); };
  rendererModInstCamData.micPlayMusicTitleCallbackFunction = [this](std::string title) { mAudioManager.playTitle(title); };

  rendererModInstCamData.micPlayWalkFootstepCallbackFunction = [this]() { mAudioManager.playWalkFootsteps(); };
  rendererModInstCamData.micPlayRunFootstepCallbackFunction = [this]() { mAudioManager.playRunFootsteps(); };
  rendererModInstCamData.micStopFootstepCallbackFunction = [this]() { mAudioManager.stopFootsteps(); };

  mStartTime = std::chrono::steady_clock::now();
  Logger::log(1, "%s: Window with OpenGL 4.6 successfully initialized\n", __FUNCTION__);
  return true;
}


void Window::mainLoop() {
  /* force VSYNC */
  glfwSwapInterval(1);

  std::chrono::time_point<std::chrono::steady_clock> loopStartTime = std::chrono::steady_clock::now();
  std::chrono::time_point<std::chrono::steady_clock> loopEndTime = std::chrono::steady_clock::now();
  float deltaTime = 0.0f;

  while (true) {
    if (!mRenderer->draw(deltaTime)) {
      break;
    }

    /* swap buffers */
    glfwSwapBuffers(mWindow);

    /* poll events in a loop */
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

  mAudioManager.cleanup();

  glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
  glfwDestroyWindow(mWindow);
  glfwTerminate();
  Logger::log(1, "%s: Terminating Window\n", __FUNCTION__);
}

std::string Window::getWindowTitle() {
  return mWindowTitle;
}

void Window::setWindowTitle(std::string newTitle) {
  mWindowTitle = newTitle;
  glfwSetWindowTitle(mWindow, mWindowTitle.c_str());
}
