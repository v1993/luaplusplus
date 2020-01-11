#pragma once
#include <stdexcept>

/**
 * @file lua++/Error.hpp
 * @brief Definition of various errors specific for lua++
*/

namespace Lua {
	/**
	 * @brief Class used for generic errors by lua++.
	*/
	class Error: public std::logic_error {
		public:
			/// Usual constructor
			explicit Error(const std::string& what_arg): logic_error(what_arg) {};
			/// Usual constructor
			explicit Error(const char* what_arg): logic_error(what_arg) {};
		};

	/**
	 * @brief Class used only for errors happened on %Lua side.
	 *
	 * This exception can be thrown if %Lua reported an error
	 * during code execution or some other internal operation.
	*/
	class StateError: public Error {
		public:
			/// Usual constructor
			explicit StateError(const std::string& what_arg): Error(what_arg) {};
			/// Usual constructor
			explicit StateError(const char* what_arg): Error(what_arg) {};
		};

	/**
	 * @brief Class used only for errors happened as result of parsing invalid data.
	 *
	 * This exception can be thrown if %Lua reported an error
	 * during code loading if it failed to parse syntax.
	*/
	class SyntaxError: public Error {
		public:
			/// Usual constructor
			explicit SyntaxError(const std::string& what_arg): Error(what_arg) {};
			/// Usual constructor
			explicit SyntaxError(const char* what_arg): Error(what_arg) {};
		};

	/**
	 * @brief Wrap function into `try/catch` block that will rethrow error as %Lua.
	 *
	 * If your function is called from %Lua side, it's very good idea to wrap it
	 * into `try/catch` block to prevent C++ exceptions from escaping it.
	 *
	 * This fuction provide simple wrapper to convert C++ exception into meaningful %Lua
	 * error message.
	 *
	 * All pure-lua errors (thrown by `lua_error`) are left unaffected.
	 *
	 * Example call:
	 *
	 * ```
	 * auto res = Lua::LuaErrorWrapper(L, myfunc, {"My arg1", arg2});
	 * ```
	 *
	 * @note Never throw non-`std::exception` to avoid troubles.
	 *
	 * @param L Lua state where error will be thrown.
	 * @param func C++ function to be called.
	 * @param args Arguments for C++ function packed into a tuple.
	 *
	 * @return Result of call to providen function.
	 * @throw Implementation-defided %Lua error in case of C++ exceptions.
	*/
	template<typename Tres, typename... Targs>
	Tres LuaErrorWrapper(lua_State* L, const std::function<Tres(Targs...)>& func, const std::tuple<Targs...>& args) {
		try {
				return std::apply(func, args);
				}
		catch (const Error& err) {
				luaL_error(L, "lua++ error: %s", err.what());
				}
		catch (const std::exception& err) {
				luaL_error(L, "C++ exception (typeid %s): %s", typeid(err).name(), err.what());
				};

		std::abort(); // We may never reach this point
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
