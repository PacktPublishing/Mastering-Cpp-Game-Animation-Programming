@REM #change current directory to this file
@%~d0
@cd %~dp0

@setlocal
setlocal enabledelayedexpansion

@echo off

if not exist "external\assimp\assimp-master" (
    git clone --depth 1 git@github.com:assimp/assimp.git external\assimp\assimp-master
) else (
    @echo already downloaded assimp
)

if not exist "external\glfw\glfw-3.4" (
    @echo "Unzipping external/glfw/glfw-3.4.zip"
    powershell -command "Expand-Archive -Force 'external\glfw\glfw-3.4.zip' 'external\glfw'"
) else (
    @echo already unzipped glfw
)

if not exist "external\glm\glm-1.0.3" (
    @echo "Unzipping external/glm/glm-1.0.3.zip"
    powershell -command "Expand-Archive -Force 'external\glm\glm-1.0.3.zip' 'external\glm'"
) else (
    @echo already unzipped glm
)

if not exist "external\imnodes\imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211" (
    @echo "Unzipping external/imnodes/imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211.zip"
    powershell -command "Expand-Archive -Force 'external\imnodes\imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211.zip' 'external\imnodes'"
) else (
    @echo already unzipped imnodes
)

if not exist "external\vk-bootstrap\vk-bootstrap-1.3.302" (
    @echo "Unzipping external/vk-bootstrap/vk-bootstrap-1.3.302.zip"
    powershell -command "Expand-Archive -Force 'external\vk-bootstrap\vk-bootstrap-1.3.302.zip' 'external\vk-bootstrap'"
) else (
    @echo already unzipped vk-bootstrap
)

if not exist "external\yaml-cpp\yaml-cpp-yaml-cpp-0.9.0" (
    @echo "Unzipping external/yaml-cpp/yaml-cpp-yaml-cpp-0.9.0.zip"
    powershell -command "Expand-Archive -Force 'external\yaml-cpp\yaml-cpp-yaml-cpp-0.9.0.zip' 'external\yaml-cpp'"
) else (
    @echo already unzipped yaml-cpp
)

if not exist "external\SDL2\SDL-release-2.30.9" (
    @echo "Unzipping external/SDL2/SDL-release-2.30.9.zip"
    powershell -command "Expand-Archive -Force 'external\SDL2\SDL-release-2.30.9.zip' 'external\SDL2'"
) else (
    @echo already unzipped SDL2
)

if not exist "external\SDL2_mixer\SDL_mixer-release-2.8.0" (
    @echo "Unzipping external/SDL2_mixer/SDL_mixer-release-2.8.0.zip"
    powershell -command "Expand-Archive -Force 'external\SDL2_mixer\SDL_mixer-release-2.8.0.zip' 'external\SDL2_mixer'"
) else (
    @echo already unzipped SDL2_mixer
)

@echo "Installiing assimp dll"
@call external\assimp\gen_vs2022.bat

@echo "Cloning SDL2 mixer deps"
@call external\download_SDL_mixer_deps.bat

@echo ========================================
@echo Setup completed successfully!
@echo ========================================
@pause