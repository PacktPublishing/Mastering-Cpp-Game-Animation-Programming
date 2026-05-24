@REM #change current directory to this file
@%~d0
@cd %~dp0

@setlocal

cmake -G "Visual Studio 17 2022" ^
	-DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo ^
	-B _build/vs2022-x64-windows ^
	.

@pause