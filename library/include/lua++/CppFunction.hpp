#pragma once
#include <functional>
#include "lua++/State.hpp"

namespace Lua {
	using CppFunction = std::function<int(Lua::StatePtr&)>;

	template<typename T>
	using CppMethod = std::function<int(T*, Lua::StatePtr&)>;

	template<typename T>
	bool checkCppType(lua_State* L, int idx, const char* name) {
		// Stack: xxx
		if ((lua_type(L, idx) == LUA_TUSERDATA) and lua_getmetatable(L, idx)) {
				// Stack: xxx, metatable for our value
				luaL_getmetatable(L, name);
				// Stack: xxx, metatable for our value, metatable for our type
				bool mteq = lua_rawequal(L, -1, -2); // Compare tables, stack unaffected
				lua_pop(L, 2);

				// Stack: xxx
				if (mteq) {
						// If metatables are the same, check for `nullptr` (if `push` failed)
						// Will most likely NEVER fail, but check to be extra sure
						auto ptr = static_cast<T**>(lua_touserdata(L, idx));
						return *ptr != nullptr;
						}
				}

		return false;
		};

	class TypeCppFunction: public TypeBase {
		private:
			static constexpr const char* tname = "CppFunction";
			static int call(lua_State*);
			static int gc(lua_State*);
		public:
			virtual void init(Lua::State& state) const override;
			virtual const std::type_info& getType() const noexcept override;
			virtual bool checkType(lua_State*, int) const noexcept override;
			virtual std::any getValue(lua_State*, int) const override;
			virtual void pushValue(lua_State*, const std::any&) const override;
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
