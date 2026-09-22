:: Suppress dumb messages, making the console prettier
@echo off


:: Creating local variables
echo Setting environment...
set BRANCH=csgo
set EXT_DIR=%cd%
set BUILD_DIR=%EXT_DIR%\build-windows
set VCVARSALL=Z:\Visual Studio 2019 Community\VC\Auxiliary\Build\vcvarsall.bat
::set VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat


:: Checking is everything okay with Visual Studio
if not exist "%VCVARSALL%" (
	echo Your Visual Studio is not a 2017 version and/or installed to other directory.
	echo You can change VCVARSALL to your own in that build.bat file.
	pause
	exit )


:: Creating build folder if not exist (only for SourceMod, MetaMod and SDK)
if not exist "%BUILD_DIR%" (
	echo Creating build-windows folder
	mkdir "%BUILD_DIR%" )


:: Initializing Visual Studio compiler variables
cd %BUILD_DIR%
if "%VSCMD_VER%"=="" (
	set MAKE=
	set CC=
	set CXX=
	call "%VCVARSALL%" x86 8.1 )


:: Getting sourcemod
echo Downloading sourcemod
if not exist "sourcemod-%BRANCH%" (
	git clone https://github.com/alliedmodders/sourcemod --recursive --single-branch sourcemod-%BRANCH%
)
cd sourcemod-%BRANCH%
set SOURCEMOD=%cd%


:: Getting metamod
echo Downloading metamod
if not exist "metamod-source-master" (
	git clone https://github.com/alliedmodders/metamod-source.git --recursive --branch master --single-branch metamod-source-master
)
cd metamod-source-master
set METAMOD=%cd%


:: Getting hl2sdk (for csgo)
echo Downloading hl2sdk-%BRANCH%
if not exist "hl2sdk-%BRANCH%" (
	git clone https://github.com/alliedmodders/hl2sdk.git --recursive  --branch %BRANCH% --single-branch hl2sdk-%BRANCH%
)
cd hl2sdk-%BRANCH%
set HL2SDK-%BRANCH%=%cd%


:: Starting a build (-m for parallel build)
echo Building...
cd %EXT_DIR%
msbuild -m msvc15/ArmsFix.sln /p:Platform="win32"
echo.
echo Check your "%EXT_DIR%\msvc15\Release" for dll!
echo.
pause
exit