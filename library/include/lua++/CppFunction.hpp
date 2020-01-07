#pragma once
#include <functional>
#include "lua++/State.hpp"

namespace Lua {
	/// Alias for `std::function<int(Lua::StatePtr&)>` AKA C++ version of `lua_CFunction`.
	using CppFunction = std::function<int(StatePtr&)>;

	/**
	 * @brief No-op wrapper around CppFunction aka `std::function<int(Lua::StatePtr&)>`.
	 *
	 * It's single propourse is to create difference between wrapped and not wrapped C++
	 * functions on C++ side.
	 *
	 * If you push `CppFunction` onto %Lua stack, it will result in userdata object with `__call`
	 * metamethod. This introduce some limitations, like inability to use it as `__call` metamethod
	 * for another object.
	 *
	 * `CppFunctionWrapper` wraps `CppFunction` with pure C function with upvalue, removing most
	 * of such limitations.
	 *
	 * @warning Function arguments will begin on stack index 2. Stack
	 * index 1 will contain `CppFunction` object itself. Please don't touch it.
	*/
	struct CppFunctionWrapper {
		CppFunction func; ///< Actual function object
		/// Wrapper around real constructor
		template<typename... Targs>
		CppFunctionWrapper(Targs... args): func(args...) {};
		/// Cast operator for easier usage in some cases.
		operator CppFunction() { return func; };
		/// Cast (reference) operator for easier usage in some cases.
		operator CppFunction& () { return func; };
		};

	/// Alias for `std::function<int(T*, Lua::StatePtr&)>`.
	template<typename T>
	using CppMethod = std::function<int(T*, StatePtr&)>;

	/// Utility functions for internal usage
	namespace CppHelpers {
		/// Cast arguments too `bool` and apply `&&` to result.
		constexpr auto CheckArgs = [](const auto& ... item) -> bool {
			return (static_cast<bool>(item) && ...);
			};
		/**
		 * @brief Apply `CheckArgs` to tuple members.
		 *
		 * This function could be useful for you as well. Example:
		 *
		 * ```
		 * auto args = Lp->get<…>(2, true);
		 * if (Lua::CppHelpers::CheckTuple(args)) {
		 *    // All arguments are valid
		 * }
		 * ```
		*/
		constexpr auto CheckTuple = [](const auto& tuple) -> bool {
			return std::apply(CheckArgs, tuple);
			};
		// Note: for type deduction only
		/// Deduce class from pointer to member function (can be combined with `std::declval` for arguments).
		template<typename R, typename C, typename... Args, typename = std::enable_if_t<std::is_member_function_pointer_v<R(C::*)(Args...)>>>
				C ClassFromFunctionPtr(R(C::*)(Args...));
		};

	/**
	 * @brief Wrap generic C++ function to version callable from %Lua.
	 *
	 * This is a very powerful mechanism which, for example, allow you to call
	 * ```
	 * std::string numberToString(lua_Number);
	 * ```
	 * from %Lua without much effort like this:
	 *
	 * ```
	 * CppFunctionNative<Lua::Number>(&numberToString);
	 * ```
	 *
	 * Abiliies and limitations:
	 * 1. Function arguments may be either values or (non-/const) l-references.
	 * 2. You must specify function arguments manually. This, however, allows you
	 * to accept and pop different types where popped one (providen in template)
	 * must be convertible to ones you accept.
	 * 3. Due to template limitations no overloaded functions are possible. But it
	 * should be rather trivial to implement them if you'll take a look at this function.
	*/
	template<typename... Targs, typename F>
	constexpr auto CppFunctionNative(F func) {
		return [func](StatePtr & Lp) -> int {
			auto args = Lp->get<Targs...>(2, true);

			if (CppHelpers::CheckTuple(args)) {
					auto doCall = [&func](auto&& ... item) {
						return std::invoke(func, *item...);
						};

					if constexpr(std::is_void_v<decltype(std::apply(doCall, args))>) {
							// Function return void
							std::apply(doCall, args);
							return 0;
							}
					else {
							// Function return stuff
							auto ret = std::apply(doCall, args);
							return Lp->push(ret);
							}
					}

			return luaL_error(**Lp, "Wrong arguments");
			};
		};

	template<typename... Targs, typename F, typename T = decltype(CppHelpers::ClassFromFunctionPtr(std::declval<F>())) >
	constexpr CppMethod<T> CppMethodNative(F func) {
		return [func](T * obj, StatePtr & Lp) -> int {
			auto args = Lp->get<Targs...>(3, true);

			if (CppHelpers::CheckTuple(args)) {
					auto doCall = [&func, &obj](auto&& ... item) {
						return std::invoke(func, obj, *item...);
						};

					if constexpr(std::is_void_v<decltype(std::apply(doCall, args))>) {
							// Function return void
							std::apply(doCall, args);
							return 0;
							}
					else {
							// Function return stuff
							auto ret = std::apply(doCall, args);
							return Lp->push(ret);
							}
					}

			return luaL_error(**Lp, "Wrong arguments");
			};
		};

	/**
	 * @brief Bind C++ CppMethod-compiant method to wrapper type.
	 * @note Result is exactly `CppMethod` and can be supplied to RTTI functions directly.
	 *
	 * ```
	 * auto x = CppMethodBind(&T::myMethod);
	 * ```
	 *
	 * @param func Pointer to member function with signature `int(StatePtr&)`.
	*/
	template<typename F, typename T = decltype(CppHelpers::ClassFromFunctionPtr(std::declval<F>())) >
	constexpr CppMethod<T> CppMethodBind(F func) { return std::bind(func, std::placeholders::_1, std::placeholders::_2); };

	/**
	 * @brief Wrapper around CppMethodBind for usage in containers.
	 *
	 * ```
	 * const Lua::MethodsTable<TestClass> TestClass::methods = {
	 *     Lua::CppMethodPair("TestMethod", &TestClass::TestMethod)
	 * }
	 *
	 * @param name String to be used as key.
	 * @param func Pointer to member function with signature `int(StatePtr&)`.
	 * ```
	*/
	template<typename F>
	constexpr auto CppMethodPair(const std::string& name, F func) {
		return std::make_pair(name, CppMethodBind(func));
		};

	template<typename... Targs, typename F, typename T = decltype(CppHelpers::ClassFromFunctionPtr(std::declval<F>())) >
	constexpr std::pair<std::string, CppMethod<T>> CppMethodNativePair(const std::string& name, F func) {
		return std::make_pair(name, CppMethodNative<Targs...>(func));
		};

	enum class cppTypeCheckResult {
		OK,
		MISMATCH,
		NULLED
		};

	/**
	 * @brief Helper function to check is given value an object of valid C++ type
	 *
	 * It checks for metatable match and makes sure that value isn't `nullptr`.
	 * @note It assume that value of userdata is of type `T*` and
	 * eiter a valid pointer or `nullptr`.
	 *
	 * @param L %Lua state to work with.
	 * @param idx Index of value to check
	 * @param name Name of type (global metatable)
	 *
	 * @return Is value at given index a valid object of requested type.
	*/
	template<typename T>
	cppTypeCheckResult checkCppType(lua_State* L, int idx, const char* name) {
		// Stack: xxx
		if ((lua_type(L, idx) == LUA_TUSERDATA) and lua_getmetatable(L, idx)) {
				// Stack: xxx, metatable for our value
				luaL_getmetatable(L, name);
				// Stack: xxx, metatable for our value, metatable for our type
				bool mteq = lua_rawequal(L, -1, -2); // Compare tables, stack unaffected
				lua_pop(L, 2);

				// Stack: xxx
				if (mteq) {
						// If metatables are the same, check for `nullptr` (if `push` failed or object finalized)
						auto ptr = static_cast<T**>(lua_touserdata(L, idx));
						return (*ptr == nullptr) ? cppTypeCheckResult::NULLED : cppTypeCheckResult::OK;
						}
				}

		return cppTypeCheckResult::MISMATCH;
		};

	/**
	 * @brief Type handler for Lua::CppFunction.
	 * @note Providen function will be called in "protected" mode (wrapped into `LuaErrorWrapper`).
	 * @warning Result is userdata with `__call` metamethod set. If you need to have a function,
	 * use TypeCppFunctionWrapper instead.
	*/
	class TypeCppFunction: public TypeBase {
			friend class TypeCppFunctionWrapper;
		private:
			static constexpr const char* tname = "CppFunction";
			static int call(lua_State*);
			static int gc(lua_State*);
		public:
			/// @copydoc TypeBase::init
			virtual void init(Lua::State&) const override;
			virtual const std::type_info& getType() const noexcept override;
			virtual bool checkType(lua_State*, int) const noexcept override;
			virtual std::any getValue(lua_State*, int) const override;
			virtual void pushValue(lua_State*, const std::any&) const override;
		};

	/**
	 * @brief Type handler for Lua::CppFunctionWrapper.
	 * @note It pushes a function, but internally uses `TypeCppFunction` to do call.
	 * Prefer that one until you MUST have a function for some reason.
	*/
	class TypeCppFunctionWrapper: public TypeBase {
		private:
			static int call(lua_State*);
			static constexpr const std::type_info& id = typeid(CppFunctionWrapper);
			// Assumption: nobody is going to store such a thing as second upvalue
			static constexpr void* getId() noexcept { return static_cast<void*>(const_cast<std::type_info*>(&id)); };
			static int luaCreate(Lua::StatePtr&);
		public:
			/// @copydoc TypeBase::init
			virtual void init(Lua::State&) const override;
			virtual const std::type_info& getType() const noexcept override;
			virtual bool checkType(lua_State*, int) const noexcept override;
			virtual std::any getValue(lua_State*, int) const override;
			virtual void pushValue(lua_State*, const std::any&) const override;
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

