cmake_minimum_required(VERSION 3.13)
project(lua++-test CXX C)
#set(CMAKE_CXX_STANDARD 17)

add_subdirectory(library EXCLUDE_FROM_ALL)

# lpeg library setup begin

# Define sources
set(LPEG_LIB_SRCS
lpeg-1.0.2/lptypes.h
lpeg-1.0.2/lpvm.c
lpeg-1.0.2/lptree.h
lpeg-1.0.2/lptree.c
lpeg-1.0.2/lpcap.h
lpeg-1.0.2/lpprint.c
lpeg-1.0.2/lpcap.c
lpeg-1.0.2/lpprint.h
lpeg-1.0.2/lpvm.h
lpeg-1.0.2/lpcode.h
lpeg-1.0.2/lpcode.c
)

# Add library
add_library(lpeg_static STATIC EXCLUDE_FROM_ALL ${LPEG_LIB_SRCS})
# Setup library to work with static Lua: build it as C++ and link with it
lua_setuplibrary(lpeg_static)

# lpeg library setup end

add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_features(lua++-test PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} lua++_static lpeg_static)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
