#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include <SDL.h>
#include <SDL_mixer.h>

class AudioManager {
  public:
    bool init();
    bool isInitialized();

    bool loadMusicFromFolder(std::string folderName, std::string extension);

    bool loadMusicTitle(std::string fileName);
    std::vector<std::string> getPlayList();
    void shuffleMusicTitles();
    void clearAllMusic();

    void setMusicVolume(int volume);
    int getMusicVolume();
    void setSoundVolume(int volume);
    int getSoundVolume();

    void playRandomMusic();
    void playTitle(std::string title);
    void playNextTitle();
    void playPrevTitle();
    void pauseMusic(bool pauseOrResume);
    void stopMusic();

    std::string getCurrentTitle();

    bool isMusicPaused();
    bool isMusicPlaying();

    bool loadWalkFootsteps(std::string fileName);
    void playWalkFootsteps(bool looping = true);

    bool loadRunFootsteps(std::string fileName);
    void playRunFootsteps(bool looping = true);

    void stopFootsteps();

    void cleanup();

  private:
    bool mInitialized = false;
    bool mMusicPlaying = false;
    bool mMusicPaused = false;
    bool mMusicAvailable = false;
    bool mWalkFootstepsAvailable = false;
    bool mRunFootstepsAvailable = false;
    bool mWalkFootstepsPlaying = false;
    bool mRunFootstepsPlaying = false;

    int mMusicVolume = 64;
    int mSoundVolume = 24;

    std::vector<std::string> mPlayList{};
    std::unordered_map<std::string, Mix_Music*> mMusicTitles{};

    Mix_Chunk* mWalkFootsteps = nullptr;
    Mix_Chunk* mRunFootsteps = nullptr;
    int mSoundChannel = -1;

    int mPlayListPosition = 0;

    void doPlayMusic();

    void musicFinishedCallback();

    /* ugly hack to work with a C-style callback */
    static AudioManager* mCurrentManager;
    static void staticMusicFinshedCallback();
};
