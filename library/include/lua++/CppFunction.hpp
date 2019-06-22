#pragma once
#include <functional>
#include "lua++/State.hpp"

namespace Lua {
	/// Alias for `std::function<int(Lua::StatePtr&)>` AKA C++ version of `lua_CFunction`.
	using CppFunction = std::function<int(Lua::StatePtr&)>;

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
		operator CppFunction&() { return func; };
		};

	/// Alias for `std::function<int(T*, Lua::StatePtr&)>`.
	template<typename T>
	using CppMethod = std::function<int(T*, Lua::StatePtr&)>;
	
	/**
	 * @brief Bind CppMethod-conformant object to callable type
	 * @note Result isn't `CppMethod`, but can be casted to it if argument is valid.
	 * 
	 * ```
	 * auto x = CppMethodBind(&T::myMethod);
	 * ```
	*/
	template<typename F>
	constexpr auto CppMethodBind(F arg) { return std::bind(arg, std::placeholders::_1, std::placeholders::_2); };
	
	template<typename F>
	constexpr auto CppMethodPair(const std::string& name, F arg) {
		return std::make_pair(name, CppMethodBind(arg));
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
