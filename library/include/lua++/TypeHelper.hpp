#pragma once
#include "lua++/CppFunction.hpp"
#include "lua++/State.hpp"
#include <cassert>

namespace Lua {

	// Template magic to call `__index` if present

	template<typename T>
	using MethodsTable = std::unordered_map<std::string, CppMethod<T>>;
	using FunctionsTable = std::unordered_map<std::string, CppFunctionWrapper>;

	/// @cond UNDOCUMENTED
	template<typename T>
	auto staticBaseIndex(StatePtr& Lp, std::shared_ptr<T> obj) -> decltype(obj->__index(Lp)) {
		auto func = CppMethodBind(&T::__index);
		return LuaErrorWrapper(**Lp, static_cast<CppMethod<T>>(func), {obj.get(), Lp});
		};

	template<typename... F>
	auto staticBaseIndex(F&& ...) -> int {
		return 0;
		};

	template<typename T>
	int staticGetConstructor(StatePtr& Lp) {
		auto obj = std::make_shared<T>(Lp);
		Lp->push(obj);
		return 1;
		};

	template<typename T>
	class TypeHelperTraits {
		private:
			template<typename T1>
			static constexpr auto checkConstructor(T1*)
			-> typename
			std::is_same <
			decltype(T1(std::declval<StatePtr&>())),
					 T1
					 >::type;

			template<typename>
			static constexpr std::false_type checkConstructor(...);

			template<typename T1>
			static constexpr auto checkMethods(T1*)
			-> typename
			std::is_same <
			decltype(T1::methods),
					 const MethodsTable<T1>
					 >::type;

			template<typename>
			static constexpr std::false_type checkMethods(...);

			template<typename T1>
			static constexpr auto checkMetamethods(T1*)
			-> typename
			std::is_same <
			decltype(T1::metamethods),
					 const FunctionsTable
					 >::type;

			template<typename>
			static constexpr std::false_type checkMetamethods(...);

			template<typename T1>
			static constexpr auto checkStaticMethods(T1*)
			-> typename
			std::is_same <
			decltype(T1::staticMethods),
					 const FunctionsTable
					 >::type;

			template<typename>
			static constexpr std::false_type checkStaticMethods(...);

			using typeConstructor = decltype(checkConstructor<T>(nullptr));
			using typeMethods = decltype(checkMethods<T>(nullptr));
			using typeMetamethods = decltype(checkMetamethods<T>(nullptr));
			using typeStaticMethods = decltype(checkStaticMethods<T>(nullptr));


		public:
			static constexpr bool haveLuaConstructor = typeConstructor::value;
			static constexpr bool haveMethods = typeMethods::value;
			static constexpr bool haveMetamethods = typeMetamethods::value;
			static constexpr bool haveStaticMethods = typeStaticMethods::value;
		};

	/// @endcond


	/*
	 * Used stuff (all optional, to be documented properly):
	 * 1. T(StatePtr&); // Lua constructor (callable by calling `static` table)
	 * 2. static const MethodsTable<T> methods; // Class methods
	 * 3. static const FunctionsTable metamethods; // Extra metamethods
	 * 4. static const FunctionsTable staticMethods; // Static methods
	 * 5. int __index(Lua::StatePtr& Lp); // User-defined overload for `__index` (called only if method isn't found)
	*/

	/**
	 * @brief Wrapper around C++ classes to make %Lua usage easier.
	 * @todo Implement built-in eqality check.
	 * @todo Make `public registrable`
	*/
	template<typename T>
	class TypeHelper: public TypeBase {
		private:
			static bool isType(lua_State* L, int idx) {
				return checkCppType<std::shared_ptr<T>>(L, idx, tname().c_str()) == cppTypeCheckResult::OK;
				};

			static void enforceType(lua_State* L, int idx) {
				switch (checkCppType<std::shared_ptr<T>>(L, idx, tname().c_str())) {
					case cppTypeCheckResult::NULLED:
						luaL_error(L, "Trying to access closed %s", tname().c_str());
						break;

					case cppTypeCheckResult::MISMATCH:
						luaL_error(L, "Trying to call on wrong object type (%s expected)", tname().c_str());
						break;

					case cppTypeCheckResult::OK:
						break;
						}
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

				if constexpr(TypeHelperTraits<T>::haveMethods) {
						if (name and T::methods.count(*name)) {
								// Create functional object with our method (object itself isn't included)
								CppFunction func = std::bind(callMethod, std::placeholders::_1, T::methods.at(*name));
								Lp->push(func);
								return 1;
								};
						}

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
						*ptr = nullptr; // To be 100% sure
						}

				return 0;
				};


			// Type's metatable
			static std::string tname() { return std::string("C++_") + typeid(T).name(); };
			// (Optional) metatable for `static` field
			// Exsist if object is %Lua-constructible
			static std::string tnameStatic() { return std::string("static_") + tname(); };

		public:

			/**
			 * @brief Push table of static methods onto stack (if exsist).
			 *
			 * @param Lp State to operate in.
			 * @return Was state pushed onto stack.
			*/
			bool pushStatic(StatePtr& Lp) const {
				if (luaL_getmetatable(**Lp, tname().c_str()) != LUA_TNIL) {
						// Type is registered
						if (lua_getfield(**Lp, -1, "static") != LUA_TNIL) {
								// Table exsist, remove mt from stack
								lua_remove(**Lp, -2);
								return true;
								};

						Lp->pop(1);
						}

				Lp->pop(1);
				return false;

				}

			/// @copydoc TypeBase::init
			void init(State& L) const override {
				// Stack: xxx
				[[maybe_unused]] auto mtok = luaL_newmetatable(L, tname().c_str());
				assert(mtok);
				// Stack: xxx, metatable

				// Add built-in `__index`
				lua_pushcclosure(L, staticIndex, 0);
				lua_setfield(L, -2, "__index");

				// Add built-in `__gc`/`__close`
				lua_pushcclosure(L, staticGc, 0);
				lua_setfield(L, -2, "__gc");
				lua_pushcclosure(L, staticGc, 0);
				lua_setfield(L, -2, "__close");

				// Add built-in `constructor`
				if constexpr(TypeHelperTraits<T>::haveLuaConstructor or TypeHelperTraits<T>::haveStaticMethods) {
						// Stack xxx, mt
						if constexpr(TypeHelperTraits<T>::haveStaticMethods)
							lua_createtable(L, T::staticMethods.size(), 0);
						else
							lua_createtable(L, 0, 0);

						// Stack: xxx, mt, static methods table
						if constexpr(TypeHelperTraits<T>::haveLuaConstructor) {
								[[maybe_unused]] auto mtok = luaL_newmetatable(L, tnameStatic().c_str());
								assert(mtok);
								// Stack: xxx, mt, static methods table, mt for static methods
								// Add `__call` with constructor
								L.push(static_cast<CppFunctionWrapper>(staticGetConstructor<T>));
								lua_setfield(L, -2, "__call");
								// Protect from changing by user
								L.push("Access not allowed");
								lua_setfield(L, -2, "__metatable");
								// Stack: xxx, mt, static methods table, mt for static methods
								L.pop(1);
								// Stack: xxx, mt, static methods table
								// Make our table callable (calls constructor)
								luaL_setmetatable(L, tnameStatic().c_str());
								// Stack: xxx, mt, static methods table
								}

						if constexpr(TypeHelperTraits<T>::haveStaticMethods) {
								L.pushDict(T::staticMethods);
								}

						// Stack: xxx, mt, static methods table
						lua_setfield(L, -2, "static");
						// Stack: xxx, mt
						}

				// Add user-defined metamethods
				if constexpr(TypeHelperTraits<T>::haveMetamethods)
					L.pushDict(T::metamethods);

				L.pop(1);
				};

			const std::type_info& getType() const noexcept override {
				return typeid(std::shared_ptr<T>);
				};

			bool checkType(lua_State* L, int idx) const noexcept override {
				return isType(L, idx);
				};

			std::any getValue(lua_State* L, int idx) const override {
				using ptrT = std::shared_ptr<T>;
				auto ptr = static_cast<ptrT**>(lua_touserdata(L, idx));
				return ptrT(**ptr);
				};

			void pushValue(lua_State* L, const std::any& obj) const override {
				using ptrT = std::shared_ptr<T>;
				auto& origPtr = std::any_cast<std::reference_wrapper<const ptrT>>(obj).get();
				/// @todo Add support for user values?
				auto newptr = static_cast<ptrT**>(lua_newuserdatauv(L, sizeof(ptrT*), 0));
				// First make sure that it is nullptr if `__gc` will be called
				*newptr = nullptr;
				// Then add `_gc`
				luaL_setmetatable(L, tname().c_str());
				// And only THEN add real data
				*newptr = new ptrT(origPtr);
				};
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

