#pragma once
#include "lua++/CppFunction.hpp"
#include "lua++/State.hpp"
#include <cassert>

namespace Lua {

	// Template magic to call `__index` if present
	template<typename T>
	auto staticBaseIndex(StatePtr& Lp, std::shared_ptr<T> obj) -> decltype(obj->__index(Lp)) {
		auto func = std::bind(&T::__index, std::placeholders::_1, std::placeholders::_2);
		return LuaErrorWrapper(**Lp, static_cast<std::function<int(T*, StatePtr&)>>(func), {obj.get(), Lp});
		};

	template<typename... F>
	auto staticBaseIndex(F&& ...) -> int {
		return 0;
		};


	template<typename T>
	class TypeHelper: public TypeBase {
		protected:
			static constexpr const char* tname() noexcept { return typeid(T).name(); };
			static bool isType(lua_State* L, int idx) {
				return checkCppType<std::shared_ptr<T>>(L, idx, tname());
				};

			static void enforceType(lua_State* L, int idx) {
				if (!isType(L, idx)) luaL_error(L, "Calling C++ method on invalid type (potential hacking attempt)");
				};

			static int staticIndex(lua_State* L) {
				// Pure Lua call
				enforceType(L, 1); // Make sure that we have valid object
				StatePtr Lp(L);
				// KDevelop crash if I use C++17 notation (in templated code), so use std::tie instead…
				// I know, this is very stupid.
				//auto [obj, name] = Lp->get<std::shared_ptr<T>, std::string>(1, true);
				std::optional<std::shared_ptr<T>> obj;
				std::optional<std::string> name;
				std::tie(obj, name) = Lp->get<std::shared_ptr<T>, std::string>(1, true);

				if (name and T::methods.count(*name)) {
						// Protect from C++ exceptions crossing Lua space
						CppFunction func = std::bind(callMethod, std::placeholders::_1, T::methods.at(*name));
						Lp->push(func);
						return 1;
						};

				return staticBaseIndex(Lp, obj.value()); // Call `__index` in base object if present
				};

			// Called by TypeCppFunc, stack 1 = function object itself
			static int callMethod(StatePtr& Lp, const CppMethod<T>& method) {
				enforceType(**Lp, 2);
				auto obj = Lp->getOne<std::shared_ptr<T>>(2);
				return method(obj.value().get(), Lp);
				};

			static int staticGc(lua_State* L) {
				// Pure Lua call
				if (isType(L, 1)) { // Valid object
						auto ptr = static_cast<std::shared_ptr<T>**>(lua_touserdata(L, 1));
						delete *ptr;
						}
				else {
						lua_warning(L, "Invalid object in destructor", false);
						}

				return 0;
				};

		public:
			void init(State &L) const override {
				// Stack: xxx
				assert((luaL_newmetatable(L, tname())));
				// Stack: xxx, metatable

				// Add built-in `__index`
				lua_pushcclosure(L, staticIndex, 0);
				lua_setfield(L, -2, "__index");

				// Add built-in `__gc`/`__close`
				lua_pushcclosure(L, staticGc, 0);
				lua_setfield(L, -2, "__gc");
				lua_pushcclosure(L, staticGc, 0);
				lua_setfield(L, -2, "__close");

				L.pushDict(T::metamethods);
				L.pop(1);
				};

			const std::type_info& getType() const noexcept override {
				return typeid(std::shared_ptr<T>);
				};

			bool checkType(lua_State* L, int idx) const noexcept override {
				return isType(L, idx);
				};

			std::any getValue(lua_State * L, int idx) const override {
				using ptrT = std::shared_ptr<T>;
				auto ptr = static_cast<ptrT**>(lua_touserdata(L, idx));
				return ptrT(**ptr);
				};

			void pushValue(lua_State * L, const std::any &obj) const override {
				using ptrT = std::shared_ptr<T>;
				auto& origPtr = std::any_cast<std::reference_wrapper<const ptrT>>(obj).get();
				/// @todo Add support for user values?
				auto newptr = static_cast<ptrT**>(lua_newuserdatauv(L, sizeof(ptrT*), 0));
				// First make sure that it is nullptr if `__gc` will be called
				*newptr = nullptr;
				// Then add `_gc`
				luaL_setmetatable(L, tname());
				// And only THEN add real data
				*newptr = new ptrT(origPtr);
				};
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
