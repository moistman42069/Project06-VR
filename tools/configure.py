"""Configure the native Android build after downloading prerequisites."""
import pathlib, subprocess, sys
root=pathlib.Path(__file__).resolve().parents[1]
subprocess.run(['cmake','-S',str(root),'-B',str(root/'build'),'-G','Ninja',
    '-DCMAKE_MAKE_PROGRAM='+str(root/'vendor/ninja/bin/ninja.exe'),
    '-DCMAKE_TOOLCHAIN_FILE='+str(root/'vendor/ndk/android-ndk-r27c/build/cmake/android.toolchain.cmake'),
    '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-29','-DANDROID_STL=c++_static',
    '-DCMAKE_BUILD_TYPE=Release','-DPython3_EXECUTABLE='+sys.executable],check=True)
