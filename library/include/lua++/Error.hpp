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
			Error(const std::string& what_arg): logic_error(what_arg) {};
			/// Usual constructor
			Error(const char* what_arg): logic_error(what_arg) {};
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
			StateError(const std::string& what_arg): Error(what_arg) {};
			/// Usual constructor
			StateError(const char* what_arg): Error(what_arg) {};
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
			SyntaxError(const std::string& what_arg): Error(what_arg) {};
			/// Usual constructor
			SyntaxError(const char* what_arg): Error(what_arg) {};
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
