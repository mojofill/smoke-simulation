@echo off
setlocal

mkdir src\spvs 2>nul

echo Compiling shaders...
glslc src\shaders\shader.vert -o src\spvs\vert.spv
glslc src\shaders\shader.frag -o src\spvs\frag.spv
glslc src\shaders\pressureSolve.comp -o src\spvs\pressureSolve.spv
glslc src\shaders\advectVelocities.comp -o src\spvs\advectVelocities.spv
glslc src\shaders\advectSmoke.comp -o src\spvs\advectSmoke.spv
glslc src\shaders\updateVelocities.comp -o src\spvs\updateVelocities.spv

echo Compiling CMake...
cmake -S . -B build -G "MinGW Makefiles" ^
  -DCMAKE_C_COMPILER=C:/MinGW/bin/gcc.exe ^
  -DCMAKE_BUILD_TYPE=Release ^
  -Dglfw3_DIR=C:/glfw-mingw/lib/cmake/glfw3

if errorlevel 1 exit /b 1

echo Building...
cmake --build build
if errorlevel 1 exit /b 1

echo Running...
.\build\vk_app.exe
