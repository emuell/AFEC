@echo off

:: Builds the crawler and model creator and tester tools:
:: Pass "/Debug" to only build debug configs, "/Release" for release configs
:: Pass "/c" to clean before building

SET CMAKE=C:\Program Files\CMake\bin\CMake.exe
IF NOT EXIST "%CMAKE%" GOTO :CMAKE_Error

SET SCRIPT_DIR=%~dp0
PUSHD %SCRIPT_DIR%

SET OUT_DIR=%SCRIPT_DIR%Out
IF NOT EXIST "%OUT_DIR%" (mkdir "%OUT_DIR%")
PUSHD %OUT_DIR%

:: process options
SET BUILD_RELEASE=1
SET BUILD_DEBUG=0

SET MSBUILD_FLAGS=-- /m /verbosity:minimal
SET CLEAN_FLAGS=

:loop
IF NOT "%1"=="" (
	IF /I "%1"=="/Debug" (
		SET BUILD_RELEASE=0
		SET BUILD_DEBUG=1
		SHIFT & GOTO :loop
	)
	IF /I "%1"=="/Release" (
		SET BUILD_DEBUG=0
		SET BUILD_RELEASE=1
		SHIFT & GOTO :loop
	)
	IF /I "%1"=="/C" (
		SET CLEAN_FLAGS=--clean-first
		SHIFT & GOTO :loop
	)
	SHIFT
	GOTO :loop
)

:: ############################################################################

:: Create x86_64 Visual Studio Solutions on Windows
SET CMAKE_GENERATOR_ARCH=x64

SET CMAKE_TARGET_OPTIONS=-DBUILD_CRAWLER=ON -DBUILD_TESTS=ON
SET SOURCE_DIR=../../Source 

:: configure
"%CMAKE%" %SOURCE_DIR% -A "%CMAKE_GENERATOR_ARCH%" %CMAKE_TARGET_OPTIONS%
if %ERRORLEVEL% GTR 0 GOTO :Build_Error

:: build
if %BUILD_DEBUG% EQU 1 (
	"%CMAKE%" --build . --config Debug %CLEAN_FLAGS% %MSBUILD_FLAGS%
	if %ERRORLEVEL% GTR 0 GOTO :Build_Error
)
if %BUILD_RELEASE% EQU 1 (
	"%CMAKE%" --build . --config Release %CLEAN_FLAGS% %MSBUILD_FLAGS%
	if %ERRORLEVEL% GTR 0 GOTO :Build_Error
)

ECHO.
ECHO *** Build succeeded ***
GOTO :end

:: ############################################################################

:MSBUILD_Error
ECHO. 
ECHO MSBUILD (%MSBUILD%) not set or is invalid
GOTO :Build_Error

:CMAKE_Error
ECHO. 
ECHO CMAKE (%CMAKE%) not set or is invalid
GOTO :Build_Error

:Build_Error
ECHO. 
ECHO *** Build failed ***

:end
