#pragma once
#include <string>
#include <typeinfo>
#include <any>
#include "lua.hpp"
#include "lua++/Number.hpp"

/**
 * @file lua++/Type.hpp
 * @brief C++/%Lua type mapping base and built-in implementations
*/

namespace Lua {
	/**
	 * @brief Abstract class describing interface for type handlers.
	 *
	 * This library does use runtime type handler specification, making whole system very flexible.
	 * While, in fact, classes are used, library rely on idea that no information is stored between calls and
	 * many lua_States and even (execution) threads may call same method at same time.
	 *
	 * Type handlers are registered in State for futher usage.
	 *
	 * Unmapped %Lua types:
	 * 1. table (for obvious reasons)
	 * 2. function (for still obvious reasons)
	 * 3. thread  (can be partially done, but who needs it anyways?!)
	 * 4. full userdata (so user can set up custom types easily)
	 * @see State::registerStandardTypes() for table of standard types and handlers.
	*/
	class State;
	class TypeBase {
		public:
			/**
			 * @brief Do generic init required for proper type usage.
			 *
			 * This function is called once when registering type to State.
			 * You must leave %Lua stack in its original state.
			 *
			 * @param L State type is being registered to.
			*/
			virtual void init([[maybe_unused]] State& L) const {};
			/**
			 * @brief Which C++ type does this handler serve.
			 * @todo Make `constexpr` in C++20.
			 *
			 * This function is called once when registering type to State.
			 * lua++ assume that result never changes.
			*/
			virtual const std::type_info& getType() const noexcept = 0;
			/**
			 * @brief Can value at given index on %Lua stack be represented as
			 * C++ type for this handler.
			 *
			 * @param L %Lua state you're working with
			 * @param idx Index of value on %Lua stack
			*/
			virtual bool checkType(lua_State* L, int idx) const noexcept = 0;
			/**
			 * @brief Is this handler most accurate for value at given index on %Lua stack.
			 *
			 * Default implementation will forward this call to `checkType`. You may want
			 * to override it if same %Lua type can be represented as different C++ ones.
			 * Check out TypeString and TypeNumber for real usage examples.
			 *
			 * @param L %Lua state you're working with
			 * @param idx Index of value on %Lua stack
			*/
			virtual bool isBestType(lua_State* L, int idx) const noexcept { return checkType(L, idx); };
			/**
			 * @brief Transform object on %Lua stack to C++ one.
			 *
			 * This function will be called only if `checkType(L, idx)`
			 * is `true`. In other cases behavior of implementation
			 * may be undefined.
			 *
			 * @param L %Lua state you're working with
			 * @param idx Index of value on %Lua stack
			*/
			virtual std::any getValue(lua_State* L, int idx) const = 0;
			/**
			 * @brief Push C++ object onto %Lua stack.
			 *
			 * @param L %Lua state you're working with
			 * @param obj `std::any` with `std::reference_wrapper<const T>` (where `T` is type handled by you).
			 * If you get `std::bad_any_cast`, try to check type of passed object with `obj.type().name()`.
			*/
			virtual void pushValue(lua_State* L, const std::any& obj) const = 0;

			virtual ~TypeBase() {};
		};

	/**
	 * @brief Type hander for C++/%Lua `bool` conversion.
	 * @todo Make all %Lua values convertible.
	*/
	class TypeBool: public TypeBase {
		public:
			virtual const std::type_info& getType() const noexcept;
			virtual bool checkType(lua_State*, int) const noexcept;
			virtual std::any getValue(lua_State*, int) const;
			virtual void pushValue(lua_State*, const std::any&) const;
		};

	/**
	 * @brief Type hander for C++/%Lua `std::string`/`string` conversion.
	 * @note It can convert %Lua number as well without alerting original value on %Lua stack.
	*/
	class TypeString: public TypeBase {
		public:
			virtual const std::type_info& getType() const noexcept;
			virtual bool checkType(lua_State*, int) const noexcept;
			virtual bool isBestType(lua_State* L, int idx) const noexcept;
			virtual std::any getValue(lua_State*, int) const;
			virtual void pushValue(lua_State*, const std::any&) const;
		};

	/**
	 * @brief Type hander for one-way C++/%Lua `const char*`/`string` conversion.
	 * @warning It can convert C++ `const char*` to %Lua `string`, but not vice versa.
	*/
	class TypeCString: public TypeBase {
		public:
			virtual const std::type_info& getType() const noexcept;
			virtual bool checkType(lua_State*, int) const noexcept;
			virtual std::any getValue(lua_State*, int) const;
			virtual void pushValue(lua_State*, const std::any&) const;
		};

	/**
	 * @brief Type hander for C++/%Lua `Lua::Number`/`number` conversion.
	*/
	class TypeNumber: public TypeBase {
		public:
			virtual const std::type_info& getType() const noexcept;
			virtual bool checkType(lua_State*, int) const noexcept;
			virtual bool isBestType(lua_State* L, int idx) const noexcept;
			virtual std::any getValue(lua_State*, int) const;
			virtual void pushValue(lua_State*, const std::any&) const;
		};

	/**
	 * @brief Type hander for C++/%Lua `nullptr(_t)`/`nil` conversion.
	*/
	class TypeNull: public TypeBase {
		public:
			virtual const std::type_info& getType() const noexcept;
			virtual bool checkType(lua_State*, int) const noexcept;
			virtual std::any getValue(lua_State*, int) const;
			virtual void pushValue(lua_State*, const std::any&) const;
		};

	/**
	 * @brief Type hander for C++/%Lua `void*`/light user data conversion.
	*/
	class TypeLightUserdata: public TypeBase {
		public:
			virtual const std::type_info& getType() const noexcept;
			virtual bool checkType(lua_State*, int) const noexcept;
			virtual std::any getValue(lua_State*, int) const;
			virtual void pushValue(lua_State*, const std::any&) const;
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
