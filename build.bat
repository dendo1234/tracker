@echo off

set tracker_version=1

REM TODO: have fun looking at compiler options

REM DESC: cl options
set cl_compiler_common_options=/nologo /W4 /wd4232 /DVERSION=%tracker_version%
set cl_linker_options=/F32768 User32.lib Shell32.lib

set cl_compiler_release_options=/O2 /Fetracker-v%tracker_version%.exe
set cl_compiler_debug_options=/Zi /Fetracker-debug.exe

REM DESC: gcc + clang options
set gcc_compiler_common_options=-Wall -Wextra -mwindows -municode -DVERSION=%tracker_version%

set gcc_compiler_release_options=-O3
set gcc_compiler_debug_options=-g

set cl_compiler_options=%cl_compiler_debug_options%
set gcc_compiler_options=%gcc_compiler_debug_options%

if "%~1"=="release" (
  set "cl_compiler_options=%cl_compiler_common_options% %cl_compiler_release_options%"
  set "gcc_compiler_options=%gcc_compiler_common_options% %gcc_compiler_release_options%"
) else (
  set "cl_compiler_options=%cl_compiler_common_options% %cl_compiler_debug_options%"
  set "gcc_compiler_options=%gcc_compiler_common_options% %gcc_compiler_debug_options%"
)

if not exist build mkdir build
pushd build

REM NOTE: triple the compilers for tripe the fun
REM remove a compiler with @REM

@echo on
clang %gcc_compiler_options% ../tracker.c -o tracker-clang.exe
gcc %gcc_compiler_options% ../tracker.c -o tracker-gcc.exe
cl %cl_compiler_options% ../tracker.c %cl_linker_options%
@echo off
popd

