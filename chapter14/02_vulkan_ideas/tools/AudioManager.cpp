#include "AudioManager.h"

#include <tuple>
#include <chrono>
#include <algorithm>
#include <random>
#include <filesystem>
#include <future>

#include "Logger.h"
#include "Tools.h"

AudioManager* AudioManager::mCurrentManager = nullptr;

bool AudioManager::init() {
  /* use SDL only for audio */
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    Logger::log(1, "%s error: unable to init SDL\n", __FUNCTION__);
    return false;
  }

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    Logger::log(1, "%s error: could not init mixer device\n", __FUNCTION__);
    Logger::log(1, "%s errormessage: %s\n", __FUNCTION__, Mix_GetError());
    Logger::log(1, "%s: disabling music output\n", __FUNCTION__);
    return false;
  }

  if (Mix_AllocateChannels(1) < 0) {
    Logger::log(1, "%s warning: could not set channel number to 1\n", __FUNCTION__);
  };

  mInitialized = true;

  /* ugly hack to work with a C-style callback */
  mCurrentManager = this;
  Mix_HookMusicFinished(staticMusicFinshedCallback);
  
  setMusicVolume(mMusicVolume);
  setSoundVolume(mSoundVolume);

  Logger::log(1, "%s: SDL audio successfully initialized\n", __FUNCTION__);
  return true;
}

void AudioManager::staticMusicFinshedCallback() {
  if (mCurrentManager)  {
    mCurrentManager->musicFinishedCallback();
  }
}

void AudioManager::musicFinishedCallback() {
  if (!mInitialized) {
    return;
  }

  if (mMusicPlaying) {
    playNextTitle();
  }
}

bool AudioManager::isInitialized() {
  return mInitialized;
}

bool AudioManager::loadMusicFromFolder(std::string folderName, std::string extension) {
  if (!mInitialized) {
    Logger::log(1, "%s error: audio not initialized, skipping music file\n", __FUNCTION__);
    return false;
  }
  if (!std::filesystem::is_directory(folderName)) {
    return false;
  }

  std::vector<std::string> playList = Tools::getDirectoryContent(folderName, extension);
  if (!playList.empty()) {
    for (const auto& filename : playList) {
      loadMusicTitle(filename);
    }
    Logger::log(1, "%s: added %i title%s to playlist\n", __FUNCTION__, playList.size(), playList.size() > 1 ? "s" : "");
  }

  return true;
}


bool AudioManager::loadMusicTitle(std::string fileName) {
  if (!mInitialized) {
    Logger::log(1, "%s error: audio not initialized, skipping music file\n", __FUNCTION__);
    return false;
  }

  Mix_Music* music = Mix_LoadMUS(fileName.c_str());

  if (!music) {
    Logger::log(1, "%s error: could not load music file '%s'\n", __FUNCTION__, fileName.c_str());
    Logger::log(1, "%s errormessage: %s\n", __FUNCTION__, Mix_GetError());
  }

  std::string cleanFileName = std::filesystem::path(fileName).filename().generic_string();


  mPlayList.emplace_back(cleanFileName);
  mMusicTitles.insert(std::make_pair(cleanFileName, music));

  mMusicAvailable = true;
  return true;
}

std::vector<std::string> AudioManager::getPlayList() {
  return mPlayList;
}

void AudioManager::shuffleMusicTitles() {
  unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(mPlayList.begin(), mPlayList.end(), std::default_random_engine(seed));
}

void AudioManager::clearAllMusic() {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  if (Mix_PlayingMusic()) {
    Mix_HaltMusic();
  }
  mPlayList.clear();

  for (auto& title : mMusicTitles) {
    Mix_FreeMusic(title.second);
  }
  mMusicTitles.clear();

  mMusicAvailable = false;
  mMusicPlaying = false;
  mMusicPaused = false;
}

void AudioManager::doPlayMusic() {
  Logger::log(1, "%s: playing title %i (%s)\n", __FUNCTION__, mPlayListPosition, mPlayList.at(mPlayListPosition).c_str());

  mMusicPaused = false;
  Mix_PlayMusic(mMusicTitles.at(mPlayList.at(mPlayListPosition)), 0);
}


void AudioManager::playRandomMusic() {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  if (mPlayList.empty()) {
    return;
  }

  mPlayListPosition = 0;
  mMusicPlaying = true;

  /* play random title */
  shuffleMusicTitles();
  doPlayMusic();
}

void AudioManager::playNextTitle() {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  int playlistSize = mPlayList.size();
  ++mPlayListPosition %= playlistSize;
  doPlayMusic();
}

void AudioManager::playPrevTitle() {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  /* negative modulo */
  int playlistSize = mPlayList.size();
  mPlayListPosition = ((--mPlayListPosition % playlistSize) + playlistSize) % playlistSize;
  doPlayMusic();
}

void AudioManager::playTitle(std::string title) {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  const auto& iter = std::find(mPlayList.begin(), mPlayList.end(), title);
  if (iter != mPlayList.end()) {
    mMusicPaused = false;
    mMusicPlaying = true;

    mPlayListPosition = std::distance(mPlayList.begin(), iter);
    doPlayMusic();
  }
}

bool AudioManager::isMusicPlaying() {
  return mMusicPlaying;
}

std::string AudioManager::getCurrentTitle() {
  if (!mInitialized || !mMusicAvailable) {
    return std::string{};
  }

  if (mPlayList.empty() || mPlayListPosition > mPlayList.size()) {
    return std::string{};
  }

  return mPlayList.at(mPlayListPosition);
}

void AudioManager::pauseMusic(bool pauseOrResume) {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  if (pauseOrResume) {
    Mix_PauseMusic();
    mMusicPaused = true;
  } else {
    Mix_ResumeMusic();
    mMusicPaused = false;
  }
}

bool AudioManager::isMusicPaused() {
  return mMusicPaused;
}

void AudioManager::stopMusic() {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  mMusicPlaying = false;
  mMusicPaused = false;

  Mix_HaltMusic();
}

void AudioManager::setMusicVolume(int volume) {
  if (!mInitialized || !mMusicAvailable) {
    return;
  }

  if (volume > 128) {
    volume = 128;
  }

  mMusicVolume = volume;
  Mix_VolumeMusic(volume);
}


int AudioManager::getMusicVolume() {
  return mMusicVolume;
}

void AudioManager::setSoundVolume(int volume) {
  if (!mInitialized) {
    return;
  }

  if (volume > 128) {
    volume = 128;
  }

  mSoundVolume = volume;
  Mix_Volume(mSoundChannel, volume);
}

int AudioManager::getSoundVolume() {
  return mSoundVolume;
}

bool AudioManager::loadWalkFootsteps(std::string fileName) {
  if (!mInitialized) {
    Logger::log(1, "%s error: audio not initialized, skipping walk footstep file\n", __FUNCTION__);
    return false;
  }
  mWalkFootsteps = Mix_LoadWAV(fileName.c_str());

  if (!mWalkFootsteps) {
    Logger::log(1, "%s: could not load footsteps from file '%s'\n", __FUNCTION__, fileName.c_str());
    Logger::log(1, "%s errormessage: %s\n", __FUNCTION__, Mix_GetError());
    return false;
  }

  mWalkFootstepsAvailable = true;
  return true;
}

void AudioManager::playWalkFootsteps(bool looping) {
  if (!mInitialized || !mWalkFootstepsAvailable) {
    return;
  }

  if (mRunFootstepsPlaying) {
    stopFootsteps();
  }

  mSoundChannel = Mix_PlayChannel(-1, mWalkFootsteps, looping ? -1 : 0);
  mWalkFootstepsPlaying = true;
}

bool AudioManager::loadRunFootsteps(std::string fileName) {
  if (!mInitialized) {
    Logger::log(1, "%s error: audio not initialized, skipping walk footstep file\n", __FUNCTION__);
    return false;
  }
  mRunFootsteps = Mix_LoadWAV(fileName.c_str());

  if (!mRunFootsteps) {
    Logger::log(1, "%s: could not load footsteps from file '%s'\n", __FUNCTION__, fileName.c_str());
    Logger::log(1, "%s errormessage: %s\n", __FUNCTION__, Mix_GetError());
    return false;
  }

  mRunFootstepsAvailable = true;
  return true;
}

void AudioManager::playRunFootsteps(bool looping) {
  if (!mInitialized || !mRunFootstepsAvailable) {
    return;
  }

  if (mWalkFootstepsPlaying) {
    stopFootsteps();
  }

  mSoundChannel = Mix_PlayChannel(-1, mRunFootsteps, looping ? -1 : 0);
  mRunFootstepsPlaying = true;
}

void AudioManager::stopFootsteps() {
  if (mWalkFootstepsPlaying == false && mRunFootstepsPlaying == false) {
    return;
  }
  mWalkFootstepsPlaying = false;
  mRunFootstepsPlaying = false;
  Mix_HaltChannel(mSoundChannel);
}

void AudioManager::cleanup() {
  mInitialized = false;
  mMusicAvailable = false;
  mWalkFootstepsAvailable = false;
  mRunFootstepsAvailable = false;

  Mix_HaltChannel(mSoundChannel);
  Mix_FreeChunk(mWalkFootsteps);
  Mix_FreeChunk(mRunFootsteps);

  clearAllMusic();

  mCurrentManager = nullptr;

  Mix_CloseAudio();
  Mix_Quit();
  SDL_Quit();
}

