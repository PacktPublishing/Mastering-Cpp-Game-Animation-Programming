@REM #change current directory to this file
@%~d0
@cd %~dp0

@REM assumpt installed "C++ Clang Compiler for Windows" ^
	@REM and "MSBuild support for LLVM(clang-cl) toolset" ^
	@REM individual components in Visual Studio Installer

cmake -G "Visual Studio 17 2022" ^
	-T "ClangCL" ^
	-DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo ^
	-B _build/vs2022-x64-windows-ClangCL ^
	.

@pause