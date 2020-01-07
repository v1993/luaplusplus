#include <functional>
#include "lua++/Type.hpp"

namespace Lua {
	// TypeBool
	const std::type_info& TypeBool::getType() const noexcept {
		return typeid(const bool);
		};

	bool TypeBool::checkType(lua_State* L, int idx) const noexcept {
		return lua_isboolean(L, idx);
		};

	std::any TypeBool::getValue(lua_State* L, int idx) const {
		return static_cast<bool>(lua_toboolean(L, idx));
		};

	void TypeBool::pushValue(lua_State* L, const std::any& arg_any) const {
		auto& b = std::any_cast<std::reference_wrapper<const bool>>(arg_any).get();
		lua_pushboolean(L, b);
		};

	// TypeString
	const std::type_info& TypeString::getType() const noexcept {
		return typeid(std::string);
		};

	bool TypeString::checkType(lua_State* L, int idx) const noexcept {
		return lua_isstring(L, idx);
		};

	bool TypeString::isBestType(lua_State* L, int idx) const noexcept {
		return lua_type(L, idx) == LUA_TSTRING;
		};

	std::any TypeString::getValue(lua_State* L, int idx) const {
		std::size_t len = 0;
		lua_pushvalue(L, idx);
		const char* data = lua_tolstring(L, -1, &len);
		auto res = std::string(data, len);
		lua_pop(L, 1);
		return res;
		};

	void TypeString::pushValue(lua_State* L, const std::any& str_any) const {
		const auto& str = std::any_cast<std::reference_wrapper<const std::string>>(str_any).get();
		lua_pushlstring(L, str.data(), str.size());
		};

	// TypeCString
	const std::type_info& TypeCString::getType() const noexcept {
		return typeid(const char*);
		};

	bool TypeCString::checkType(lua_State* L, int idx) const noexcept {
		return false;
		};

	std::any TypeCString::getValue(lua_State* L, int idx) const {
		return std::any();
		};

	void TypeCString::pushValue(lua_State* L, const std::any& str_any) const {
		const char* str = std::any_cast<std::reference_wrapper<const char* const>>(str_any); // Insanity as is
		lua_pushstring(L, str);
		};

	// TypeNumber
	const std::type_info& TypeNumber::getType() const noexcept {
		return typeid(const Number);
		};

	bool TypeNumber::checkType(lua_State* L, int idx) const noexcept {
		return lua_isnumber(L, idx);
		};

	bool TypeNumber::isBestType(lua_State* L, int idx) const noexcept {
		return lua_type(L, idx) == LUA_TNUMBER;
		};

	std::any TypeNumber::getValue(lua_State* L, int idx) const {
		if (lua_isinteger(L, idx)) {
				return Number(lua_tointeger(L, idx));
				}
		else {
				return Number(lua_tonumber(L, idx));
				};
		};

	void TypeNumber::pushValue(lua_State* L, const std::any& str_any) const {
		auto& num = std::any_cast<std::reference_wrapper<const Number>>(str_any).get();

		if (num.isInteger()) {
				lua_pushinteger(L, (lua_Integer)num);
				}
		else {
				lua_pushnumber(L, (lua_Number)num);
				}
		};
	/*
		// TypeLNumber
		const std::type_info& TypeLNumber::getType() const noexcept {
			return typeid(const lua_Number);
		};

		bool TypeLNumber::checkType(lua_State* L, int idx) const noexcept {
			return false;
		};

		std::any TypeLNumber::getValue(lua_State* L, int idx) const {
			return std::any();
		};

		void TypeLNumber::pushValue(lua_State* L, const std::any& str_any) const {
			auto& num = std::any_cast<std::reference_wrapper<const lua_Number>>(str_any).get();
			lua_pushnumber(L, num);
		};

		// TypeLInteger
		const std::type_info& TypeLInteger::getType() const noexcept {
			return typeid(const lua_Integer);
		};

		bool TypeLInteger::checkType(lua_State* L, int idx) const noexcept {
			return false;
		};

		std::any TypeLInteger::getValue(lua_State* L, int idx) const {
			return std::any();
		};

		void TypeLInteger::pushValue(lua_State* L, const std::any& str_any) const {
			auto& num = std::any_cast<std::reference_wrapper<const lua_Integer>>(str_any).get();
			lua_pushinteger(L, num);
		};
	*/
	// TypeNull
	const std::type_info& TypeNull::getType() const noexcept {
		return typeid(const std::nullptr_t);
		};

	bool TypeNull::checkType(lua_State* L, int idx) const noexcept {
		return lua_isnil(L, idx);
		};

	std::any TypeNull::getValue(lua_State* L, int idx) const {
		return nullptr;
		};

	void TypeNull::pushValue(lua_State* L, const std::any& str_any) const {
		lua_pushnil(L);
		};

	// TypeLightUserdata
	const std::type_info& TypeLightUserdata::getType() const noexcept {
		return typeid(void* const);
		};

	bool TypeLightUserdata::checkType(lua_State* L, int idx) const noexcept {
		return lua_islightuserdata(L, idx);
		};

	std::any TypeLightUserdata::getValue(lua_State* L, int idx) const {
		return lua_touserdata(L, idx);
		};

	void TypeLightUserdata::pushValue(lua_State* L, const std::any& arg_any) const {
		auto& ptr = std::any_cast<std::reference_wrapper<void* const>>(arg_any).get();
		lua_pushlightuserdata(L, ptr);
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
