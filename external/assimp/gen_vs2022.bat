@REM #change current directory to this file
@%~d0
@cd %~dp0

cmake -G "Visual Studio 17 2022" ^
	-B _build/vs2022-x64-windows ^
	-DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo ^
	-DBUILD_SHARED_LIBS=ON ^
	-DASSIMP_BUILD_TESTS=OFF ^
	-DCMAKE_BUILD_TYPE=RelWithDebInfo ^
	assimp-master

cmake --build _build/vs2022-x64-windows --config RelWithDebInfo

@pause