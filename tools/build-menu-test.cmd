@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Isrc /Ivendor\font8x8 /Ivendor\OpenXR-SDK\include /Ivendor\unity-xr-plugin\CommonHeaders\ProviderInterface tests\menu_test.cpp src\vr_options.cpp /Foout\ /Feout\menu_test.exe
if errorlevel 1 exit /b 1
out\menu_test.exe
exit /b %errorlevel%
