#include "lua++/CppFunction.hpp"
#include "lua++/Error.hpp"
#include <cassert>

namespace Lua {

	int TypeCppFunction::gc(lua_State* L) {
		if (checkCppType<CppFunction>(L, 1, tname)) { // Valid object
				auto ptr = static_cast<CppFunction**>(lua_touserdata(L, 1));
				delete *ptr;
				}
		else {
				lua_warning(L, "Invalid CppFunction in destructor", false);
				}

		return 0;
		};

	int TypeCppFunction::call(lua_State* L) {
		if (checkCppType<CppFunction>(L, 1, tname)) { // Valid object
				auto ptr = static_cast<CppFunction**>(lua_touserdata(L, 1));
				auto Lp = StatePtr(L);
				return LuaErrorWrapper(L, **ptr, {Lp});
				}
		else {
				luaL_error(L, "CppFunction call");
				return 0;
				}
		};

	void TypeCppFunction::init(Lua::State& L) const {
		assert((luaL_newmetatable(L, tname)));

		lua_pushcclosure(L, call, 0);
		lua_setfield(L, -2, "__call");
		lua_pushcclosure(L, gc, 0);
		lua_setfield(L, -2, "__gc");
		lua_pushcclosure(L, gc, 0);
		lua_setfield(L, -2, "__close");
		L.pop(1);
		};

	const std::type_info& TypeCppFunction::getType() const noexcept {
		return typeid(CppFunction);
		};

	bool TypeCppFunction::checkType(lua_State* L, int idx) const noexcept {
		return checkCppType<CppFunction>(L, idx, tname);
		};

	std::any TypeCppFunction::getValue(lua_State* L, int idx) const {
		auto ptr = static_cast<CppFunction**>(lua_touserdata(L, idx));
		return CppFunction(**ptr);
		};

	void TypeCppFunction::pushValue(lua_State* L, const std::any& obj) const {
		auto& origFunc = std::any_cast<std::reference_wrapper<const CppFunction>>(obj).get();
		// Based on as TypeHelper
		auto newptr = static_cast<CppFunction**>(lua_newuserdatauv(L, sizeof(CppFunction*), 0));
		// First make sure that it is nullptr if `__gc` will be called
		*newptr = nullptr;
		// Then add `_gc`
		luaL_setmetatable(L, tname);
		// And only THEN add real data
		*newptr = new CppFunction(origFunc);
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
