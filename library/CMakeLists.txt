# Pure lua

enable_language(C CXX)

set(LUA_LIB_SRCS
	lua/lapi.c
	lua/lauxlib.c
	lua/lbaselib.c
	lua/lcode.c
	lua/lcorolib.c
	lua/lctype.c
	lua/ldblib.c
	lua/ldebug.c
	lua/ldo.c
	lua/ldump.c
	lua/lfunc.c
	lua/lgc.c
	lua/linit.c
	lua/liolib.c
	lua/llex.c
	lua/lmathlib.c
	lua/lmem.c
	lua/loadlib.c
	lua/lobject.c
	lua/lopcodes.c
	lua/loslib.c
	lua/lparser.c
	lua/lstate.c
	lua/lstring.c
	lua/lstrlib.c
	lua/ltable.c
	lua/ltablib.c
	lua/ltests.c
	lua/ltm.c
	lua/lundump.c
	lua/lutf8lib.c
	lua/lvm.c
	lua/lzio.c
)

set(LUA_LIB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/lua")
SET_SOURCE_FILES_PROPERTIES(${LUA_LIB_SRCS} PROPERTIES LANGUAGE CXX)

add_library(lua_static STATIC ${LUA_LIB_SRCS})
set_target_properties(lua_static PROPERTIES LANGUAGE CXX)
set_target_properties(lua_static PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(lua_static PUBLIC cxx_std_17)
target_include_directories(lua_static PUBLIC "${LUA_LIB_INCLUDE}")

# FIXME: add defenitions
set(LUA_DEFINITIONS LUA_USE_POSIX)
target_compile_definitions(lua_static PUBLIC ${LUA_DEFINITIONS})

# lua++

add_library(lua++_static STATIC
	src/State.cpp
	src/Type.cpp)

target_link_libraries(lua++_static lua_static)
target_compile_features(lua++_static PUBLIC cxx_std_17)
target_include_directories(lua++_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
