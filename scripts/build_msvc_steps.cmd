@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
cd /d H:\Ai\hermes-desktop\tb303-juce
if exist build rmdir /s /q build
echo [JUCE-MSVC] Configure
cmake -B build -G "Visual Studio 18 2026" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_CXX_STANDARD=17 ^
  -DJUCE_DIR="H:/Ai/hermes-desktop/JUCE" ^
  .
echo [JUCE-MSVC] Build
cmake --build build --config Release
endlocal
