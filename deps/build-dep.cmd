@echo off

:vs_env
rem Choise correct Visual Studio Home Path
rem set VS_HOME=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC
rem set VS_HOME=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build
set VS_HOME=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build
call "%VS_HOME%\vcvarsall.bat" x86_amd64

rem The correct version must be specified IF got 'error MSB8020', Run 'cmake -G' to see available list.
rem set TARGET_VS_VERSION="Visual Studio 14 2015 Win64"
rem set TARGET_VS_VERSION="Visual Studio 15 2017 Win64"
set TARGET_VS_VERSION="Visual Studio 16 2019"

set deps_home=%CD%
pushd ..\bin
set bin_home=%CD%
goto build_iir

:build_glfw
cd %bin_home%
set glfw_home=%deps_home%\glfw
md glfw
cd glfw
cmake -G %TARGET_VS_VERSION%  %glfw_home%
cmake --build . --config Release
mv src/Release/glfw3.lib %bin_home%/
cd %bin_home%
rm -rf glfw
if ERRORLEVEL 1 goto end
goto end

:buld_glad
goto end

:build_FTXUI
cd %bin_home%
set ftxui_home=%deps_home%\FTXUI
md ftxui
cd ftxui
cmake %deps_home%
cmake --build . --config Release
mv Release/*.lib %bin_home%/
cd %bin_home%
rm -rf ftxui
if ERRORLEVEL 1 goto end
goto end

:build_rtaudio
set rt_home=%deps_home%\rtaudio
cd %bin_home%
rm -rf rtaudio
md rtaudio
cd rtaudio
cmake -DBUILD_SHARED_LIBS=OFF -DRTAUDIO_BUILD_STATIC_LIBS=ON -DCMAKE_CXX_FLAGS=/MD -DMSVC=ON -DRTAUDIO_STATIC_MSVCRT=ON %rt_home%
rem 需要打开 vs 修改为 /MD
cmake --build . --config Release 
mv Release/rtaudio.* %bin_home%/
if ERRORLEVEL 1 goto end
goto end

:build_r8brain
rem 这个库不工作, 导致 rtaudio 线程异常
set rt_home=%deps_home%\r8brain
cd %bin_home%
md r8brain
cd r8brain
cl /c /O2 /MD %rt_home%\r8bbase.cpp
mv *.obj  %bin_home%/
goto end

:build_libsamplerate
set lib_home=%deps_home%\libsamplerate
cd %bin_home%
md libsamplerate
cd libsamplerate
cmake %lib_home%
cmake --build . --config Release
mv src/Release/*.lib  %bin_home%/
goto end

:build_iir
set lib_home=%deps_home%\iir1
cd %bin_home%
rm -rf iir1 
md iir1
cd iir1
cmake %lib_home%
cmake --build . --config Release
mv iir1/Release/iir_static.lib %bin_home%/
goto end

:end
if ERRORLEVEL 1 pause
exit