# lua++

This is an application-oriented C++ Lua bridge.

This library is currnetly in alpha state. However, I try
to keep docs actual, so you can go to `library/` and run

```
doxygen Doxyfile
```
to generate HTML docs.

See `src/main.cpp` for example usages.

# Adding pure C library into your project

LPeg is included as an example of how C library be included into your project.

Please keep in mind that this will work this easily only with pure C libraries,
and even then some will refuse to build in C++ mode. LPeg works just fine.

General idea is to do this in CMake:

```CMake
add_library(mylualib_static STATIC
	YOUR.c
	SOURCES.c
	HERE.h)

lua_setuplibrary(mylualib_static) # Set C++ build mode and link to static Lua
target_link_libraries(${PROJECT_NAME} lua++_static mylualib_static) # <= Make sure to link with it
```

Then, in your C++ code:

```C++
int luaopen_mylualib(lua_State* L);

…
	L.addPreloaded("mylualib", luaopen_mylualib);
```

And load it as usual in Lua!

```Lua
local mylualib = require 'mylualib'
```

Note that it is **very** recommended to strip package library to avoid loading external copy instead.
