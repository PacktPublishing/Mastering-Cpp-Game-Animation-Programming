#!/bin/sh
# change current directory to this file

SCRIPT_PATH="$(dirname "$0")"
cd "$SCRIPT_PATH" || exit 1

if [ ! -d "external/assimp/assimp-master" ]; then
    git clone --depth 1 git@github.com:assimp/assimp.git external/assimp/assimp-master
else
    echo "already downloaded assimp"
fi

if [ ! -d "external/glfw/glfw-3.4" ]; then
    echo "Unzipping external/glfw/glfw-3.4.zip"
    unzip -q "external/glfw/glfw-3.4.zip" -d "external/glfw"
else
    echo "already unzipped glfw"
fi

if [ ! -d "external/glm/glm-1.0.3" ]; then
    echo "Unzipping external/glm/glm-1.0.3.zip"
    unzip -q "external/glm/glm-1.0.3.zip" -d "external/glm"
else
    echo "already unzipped glm"
fi

if [ ! -d "external/imnodes/imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211" ]; then
    echo "Unzipping external/imnodes/imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211.zip"
    unzip -q "external/imnodes/imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211.zip" -d "external/imnodes"
else
    echo "already unzipped imnodes"
fi

if [ ! -d "external/vk-bootstrap/vk-bootstrap-1.3.302" ]; then
    echo "Unzipping external/vk-bootstrap/vk-bootstrap-1.3.302.zip"
    unzip -q "external/vk-bootstrap/vk-bootstrap-1.3.302.zip" -d "external/vk-bootstrap"
else
    echo "already unzipped vk-bootstrap"
fi

if [ ! -d "external/yaml-cpp/yaml-cpp-yaml-cpp-0.9.0" ]; then
    echo "Unzipping external/yaml-cpp/yaml-cpp-yaml-cpp-0.9.0.zip"
    unzip -q "external/yaml-cpp/yaml-cpp-yaml-cpp-0.9.0.zip" -d "external/yaml-cpp"
else
    echo "already unzipped yaml-cpp"
fi

if [ ! -d "external/SDL2/SDL-release-2.30.9" ]; then
    echo "Unzipping external/SDL2/SDL-release-2.30.9.zip"
    unzip -q "external/SDL2/SDL-release-2.30.9.zip" -d "external/SDL2"
else
    echo "already unzipped SDL2"
fi

if [ ! -d "external/SDL2_mixer/SDL_mixer-release-2.8.0" ]; then
    echo "Unzipping external/SDL2_mixer/SDL_mixer-release-2.8.0.zip"
    unzip -q "external/SDL2_mixer/SDL_mixer-release-2.8.0.zip" -d "external/SDL2_mixer"
else
    echo "already unzipped SDL2_mixer"
fi

# just use os package instead of build manually
# echo "Installing assimp"
# cd "$SCRIPT_PATH/external/assimp"
# make install

echo "Cloning SDL2 mixer deps"
cd "$SCRIPT_PATH/external/SDL2_mixer"
chmod +x download_SDL_mixer_deps.sh
./download_SDL_mixer_deps.sh
cd "$SCRIPT_PATH"

echo ========================================
echo Setup completed successfully!
echo ========================================

#kind of pause
read -p "Press [Enter] key to continue..."