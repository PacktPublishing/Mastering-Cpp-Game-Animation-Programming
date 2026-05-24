@REM #change current directory to this file
@%~d0
@cd %~dp0

@REM dl link: https://github.com/libsdl-org/SDL_mixer/archive/refs/tags/release-2.8.0.zip
@REM unzip to this folder, then run this .bat
	@REM cuz SDL_mixer need download deps manually
	@REM you could Run the "download.sh" script in the "SDL_mixer-release-2.8.0/external" folder
		@REM or just run this .bat is same effect of the "download.sh"

@echo off
setlocal enabledelayedexpansion

set MY_SDL2_MIXER_ROOT=SDL_mixer-release-2.8.0
cd %MY_SDL2_MIXER_ROOT%

set "step=0"
for /f "usebackq delims=" %%a in (`type .gitmodules ^| findstr /v /c:"["`) do (
    set /a step+=1
    set "line=%%a"
    
    for /f "tokens=1,* delims==" %%b in ("!line!") do (
        set "value=%%c"
        for /f "tokens=*" %%d in ("!value!") do set "value=%%d"
    )
    
    if !step! equ 1 set "repo_path=!value!"
    if !step! equ 2 set "repo_url=!value!"
    if !step! equ 3 (
        set "repo_branch=!value!"
        if not exist "!repo_path!\.git" (
            if exist "!repo_path!" rmdir /s /q "!repo_path!"
            git clone --recursive !repo_url! !repo_path! -b !repo_branch!
        ) else (
			echo Skipping !repo_path! - already exists
		)
        set "step=0"
    )
)


@echo All %MY_SDL2_MIXER_ROOT% external modules cloned successfully!!

@pause