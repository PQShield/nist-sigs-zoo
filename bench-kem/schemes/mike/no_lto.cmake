# Loaded right after upstream's project() call (CMAKE_PROJECT_INCLUDE, see Makefile).
# Upstream's .cmake/flags.cmake sets CMAKE_INTERPROCEDURAL_OPTIMIZATION (LTO) for every
# build type but Debug, and CMake puts the IPO compile options (-flto=auto
# -fno-fat-lto-objects) after all other flags. Empty IPO options keep LTO off.
set(CMAKE_C_COMPILE_OPTIONS_IPO "")
