#include "lua++/CppFunction.hpp"
#include "lua++/Error.hpp"
#include <cassert>

namespace Lua {

	int TypeCppFunction::gc(lua_State* L) {
		if (checkCppType<CppFunction>(L, 1, tname) == cppTypeCheckResult::OK) { // Valid object
				auto ptr = static_cast<CppFunction**>(lua_touserdata(L, 1));
				delete *ptr;
				*ptr = nullptr;
				}

		// No worries, it's probably just closed

		return 0;
		};

	int TypeCppFunction::call(lua_State* L) {
		if (checkCppType<CppFunction>(L, 1, tname) == cppTypeCheckResult::OK) { // Valid object
				auto ptr = static_cast<CppFunction**>(lua_touserdata(L, 1));
				auto Lp = StatePtr(L);
				return LuaErrorWrapper(L, **ptr, {Lp});
				}
		else {
				luaL_error(L, "Call to closed or invalid CppFunction");
				return 0;
				}
		};

	void TypeCppFunction::init(Lua::State& L) const {
		[[maybe_unused]] auto mtok = luaL_newmetatable(L, tname);
		assert(mtok);

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
		return checkCppType<CppFunction>(L, idx, tname) == cppTypeCheckResult::OK;
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

	const std::type_info& TypeCppFunctionWrapper::getType() const noexcept {
		return typeid(CppFunctionWrapper);
		};

	bool TypeCppFunctionWrapper::checkType(lua_State* L, int idx) const noexcept {
		// Stack: xxx
		auto res = false;

		if (lua_iscfunction(L, idx)) {
				if (lua_getupvalue(L, idx, 2)) {
						// Stack: xxx, upvalue
						if (lua_islightuserdata(L, -1)) {
								res = lua_touserdata(L, -1) == getId();
								}

						lua_pop(L, 1);
						// Stack: xxx
						};
				};

		return res;
		};

	std::any TypeCppFunctionWrapper::getValue(lua_State* L, int idx) const {
		// Stack: xxx
		lua_getupvalue(L, idx, 1);
		// Stack: xxx, upvalue
		auto res = StatePtr(L)->getOne<CppFunction>(-1);
		lua_pop(L, 1);
		// Stack: xxx
		return CppFunctionWrapper(res.value());
		};

	int TypeCppFunctionWrapper::call(lua_State* L) {

		// Get function, push it into first place on stack, forward call
		lua_pushvalue(L, lua_upvalueindex(1));
		lua_insert(L, 1);
		return TypeCppFunction::call(L);
		};

	int TypeCppFunctionWrapper::luaCreate(Lua::StatePtr& Lp) {
		auto nargs = lua_gettop(**Lp) - 1;

		for (int i = 1; i <= nargs; ++i) {
				auto func = Lp->getOne<CppFunction>(i + 1);

				if (func) Lp->push(CppFunctionWrapper(*func));
				else      Lp->push(nullptr);
				};

		return nargs;
		};

	void TypeCppFunctionWrapper::init(Lua::State& L) const {
		L.push(static_cast<CppFunction>(luaCreate));
		lua_setglobal(L, "CppFunctionWrapper");
		};

	void TypeCppFunctionWrapper::pushValue(lua_State* L, const std::any& obj) const {
		auto& origFunc = std::any_cast<std::reference_wrapper<const CppFunctionWrapper>>(obj).get();
		// Push internal function
		StatePtr(L)->push<CppFunction>(origFunc.func);
		lua_pushlightuserdata(L, getId());
		// Pop internal function, push wrapper
		lua_pushcclosure(L, call, 2);
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
