#pragma once
#include <variant>
#include "lua.hpp"

/**
 * @file lua++/Number.hpp
 * @brief Helper tools to work with %Lua numbers
*/

namespace Lua {
	/**
	 * @brief Class used to represent %Lua number in C++.
	 *
	 * This is a helper class to work with numbers.
	 * When you get number from lua side, you want to
	 * handle it simply in most cases.
	 *
	 * This class allow you to store number precisely,
	 * while making it possible to cast it to various
	 * real representations.
	*/
	class Number {
		protected:
			/**
			 * @brief Internal holder of value (either `lua_Number` or `lua_Integer`).
			 */
			std::variant<lua_Number, lua_Integer> holder;
		public:
			Number() = delete;
			/**
			 * @brief Constructor from lua_Number (for non-integers).
			 * @param num Non-integer number to be stored.
			 */
			Number(lua_Number  num): holder(num) {};
			/**
			 * @brief Copy constructor.
			 * @param old Value being copied.
			 */
			Number(const Number& old): holder(old.holder) {};
			/**
			 * @brief Move constructor.
			 * @param old Value being moved.
			 */
			Number(Number&& old): holder(std::move(old.holder)) {};

			/**
			 * @brief Check is actual value an integer.
			 * @return Is contained value an `lua_Integer`.
			 */
			bool isInteger() const noexcept { return std::holds_alternative<lua_Integer>(holder); };

			/**
			 * @brief Get internal holder.
			 * @warning This function is not supposed to be used widely.
			 * @return `std::variant<lua_Number, lua_Integer>`, used internally.
			 */
			std::variant<lua_Number, lua_Integer> getInternal() const noexcept { return holder; };

			/**
			 * @brief Cast operator to `lua_Number`.
			 * @warning It's recommended to use explicit cast to avoid sudden results.
			 * @return Accurate value if containes `lua_Number`, casted one otherwise.
			 */
			operator lua_Number() const noexcept {
				return std::holds_alternative<lua_Number>(holder)  ? std::get<lua_Number>(holder)  : std::get<lua_Integer>(holder);
				};
			/**
			 * @brief Cast operator to `lua_Integer`.
			 * @warning It's recommended to use explicit cast to avoid sudden results.
			 * @return Accurate value if containes `lua_Integer`, casted one otherwise.
			 */
			operator lua_Integer() const noexcept {
				return std::holds_alternative<lua_Integer>(holder) ? std::get<lua_Integer>(holder) : std::get<lua_Number>(holder);
				};

			/**
			 * @brief Unary minus, most useful with literals.
			 * @note It preserves internal number type.
			 * @return New `Number` object holding result of application unary minus to internal type.
			 */
			Number operator-() {
				if (isInteger())
					return Number(-std::get<lua_Integer>(holder));
				else
					return Number(-std::get<lua_Number>(holder));
				};
		};

	namespace NumberLiterals {
		/**
		 * @namespace NumberLiterals
		 * @brief Literals for usage with Lua::Number.
		 *
		 * This namespace provide numeric literals for
		 * easy creation of Lua::Number objects. See example below:
		 * ```
		 * using Lua::NumberLiterals; // Required to activate this feature
		 * 5.3_ln  // Create Lua::Number holding lua_Number
		 * 42_li   // Create Lua::Number holding lua_Integer
		 * -5.4_ln // Works as expected thanks to unary minus operator
		 * -666_li // Same here
		 * ```
		*/

		/// Literal for %Lua numbers
		inline Number operator "" _ln(long double num) {
			return Number((lua_Number)num);
			};

		/// Literal for %Lua integers
		inline Number operator "" _li(unsigned long long int num) {
			return Number((lua_Integer)num);
			};
		}
	}
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
