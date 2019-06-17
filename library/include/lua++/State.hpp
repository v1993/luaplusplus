#pragma once
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <sstream>
#include <functional>

#include "lua++/Type.hpp"
#include "lua.hpp"

/**
 * @file lua++/State.hpp
 * @brief C++ warapper around lua_State and helpers
*/

namespace Lua {
	/**
	 * @brief Set which libs will be preloaded in Lua::State.
	*/
	enum class DefaultLibsPreset {
		NONE,              ///< No libraries or type handlers.
		BASE,              ///< Only global functions (`base`).
		SAFE,              ///< Common libraries: `base`, `coroutine`, `table`, `string`, `math`, `utf8`.
		SAFE_WITH_PACKAGE, ///< Same as `SAFE`, but also loads `package`.
		ALL                ///< All Lua libraries: `SAFE_WITH_PACKAGE` + `io`, `os` and `debug`.
		};

	/**
	 * @brief %Lua built-in libraries.
	*/
	enum class DefaultLibs {
		BASE, ///< Base libarary aka `_G` (global functions)
		COROUTINE, ///< `coroutine` library
		TABLE, ///< `table` liblibrary
		STRING, ///< `string` library
		MATH, ///< `math` library
		UTF8, ///< `utf8` library
		PACKAGE, ///< `package` library
		IO, ///< `io` library
		OS, ///< `os` library
		DEBUG ///< `debug` library
		};

	/**
	 * @brief Mode specificator for %Lua code load related-functions.
	 *
	 * @warning According to %Lua docs,
	 * > %Lua does not check the consistency of binary chunks. Maliciously crafted binary chunks can crash the interpreter.
	*/
	enum class LoadMode {
		TEXT, ///< Text chunks only
		BINARY, ///< Binary chunks only
		BOTH ///< Both text and binary chunks
		};

	/**
	 * @brief %Lua state manager
	 *
	 * This is a central class of lua++ library. It is C++ wrapper around `lua_State*`.
	 * Most functional of this library relies on fact that they are used in %Lua state
	 * managed by this class.
	*/
	class State {
			friend class StatePtr;
		protected:
			lua_State* state = nullptr; ///< lua_State being currently managed (coroutine-specific).
			lua_State* mainState = nullptr; ///< main lua_State.
			std::unordered_map<std::type_index, std::shared_ptr<TypeBase>> knownTypes; ///< Map of C++ types to their handlers.
			std::vector<std::shared_ptr<TypeBase>> knownTypesList; ///< List of allregistered handlers.

			State** luaStatePtr = nullptr; ///< Location of pointer to this State to be used by getFromLuaState.

			std::stringstream warnBuf; ///< Buffer for accumulating warning message parts.
			std::function<void(const std::string&)> warnFunc; ///< Function to be called on warning message.

			std::optional<std::any> getGeneric(int idx); ///< Helper function for get/getOne which try to deduce type of Lua value.

			/**
			 * @brief Update Lua-side pointers to current object.
			 *
			 * This include:
			 * 1. Pointer stored in "%Lua extra space"
			 * 2. Pointer for warning function
			 *
			 * This function is called by all constructors.
			 * @warning This function call `lua_setwarnf`. This
			 * shouldn't affect your code if you set warnings handler
			 * via `setWarningFunction()`, but will override one set using `lua_setwarnf`;
			*/
			void updateStatePointer();
			/**
			 * @brief Return type handler for requested type.
			 *
			 * This will perform lookup in `knownTypes` for requested type.
			 * @return Handler for requested type.
			 * @throw Lua::Error Handler wasn't found.
			*/
			const std::shared_ptr<TypeBase>& getTypeHandler(const std::type_info&) const;
			/**
			 * @brief Turn %Lua error into Lua::StateError
			 *
			 * It pops top element from %Lua stack and uses it to create
			 * meaningful `.what()` message.
			 * @throw Lua::StateError with description of error
			*/
			[[noreturn]] void throwLuaError();

			static void warnHandler(void *ud, const char *msg, int tocont); ///< Append message to buffer and/or call user warning handler
		public:
			/**
			 * @brief Get State object from `lua_State*`.
			 *
			 * This function get State used for its management.
			 * Pointer is stored in %Lua extra space and is set by updateStatePointer()
			 * (called internally in constructors).
			 * @warning This function may lead to segfault if used with state not managed by State class.
			 * @warning Resulting State will work with main thread even if called for coroutine state. To
			 * overpass this issue, use StatePtr instead.
			 * @param L %Lua state to be retrived
			*/
			static State* getFromLuaState(lua_State* L) { return **static_cast<State***>(lua_getextraspace(L)); };

			/**
			 * @brief Main constructor.
			 *
			 * @param openlibs Check out DefaultLibsPreset description to know what different values do.
			*/
			State(DefaultLibsPreset openlibs = DefaultLibsPreset::SAFE_WITH_PACKAGE);
			State(const State&) = delete; ///< Explicitly deleted to prevent copy.
			/// Move constructor.
			State(State&& old):
				state(std::move(old.state)),
				mainState(std::move(old.mainState)),
				knownTypes(std::move(old.knownTypes)),
				knownTypesList(std::move(old.knownTypesList)),
				luaStatePtr(std::move(old.luaStatePtr)),
				warnBuf(std::move(old.warnBuf)),
				warnFunc(std::move(old.warnFunc)) {
				// To avoid double-free and fail on misuse
				old.state = nullptr;
				old.mainState = nullptr;
				updateStatePointer();
				};
			virtual ~State(); ///< Destructor (declared `virtual`) to allow inheritance.

			/// @name Most common Lua functions
			/// @{

			/// Alias for `lua_pop`.
			void pop(int idx) { lua_pop(state, idx); };

			/**
			 * @brief Perform protected call to %Lua.
			 *
			 * This is a C++-style wrapper around `lua_pcall`.
			 *
			 * If called with `2` as first argument:
			 *
			 * %Lua stack before call:
			 * ```
			 * xxx | func | arg1 | arg2
			 * ```
			 * This will end up with calling `func` with arguments `arg1` and `arg2`.
			 *
			 * %Lua stack after (successful) call:
			 * ```
			 * xxx | res1 | res2
			 * ```
			 * Where `res1` and `res2` are return values from called function
			 *
			 * @warning Return value meaning differ from `lua_pcall`!
			 * @return Amount of arguments left on stack after call (equal to `nres` if provided)
			 * @param nargs Amount of argument function takes (see description on how to pass them)
			 * @param nres If provided, amount of results on stack as result of succesful call
			 * (with stripping out extra ones and appending to required count with `nil`).
			 * If not provided, `LUA_MULTRET` will be passed as value to `lua_pcall` and amount of results
			 * is returned.
			 * @throw StateError Call resulted in `LUA_ERRRUN` (calls throwLuaError() to form a message).
			 * @throw std::bad_alloc Call resulted in `LUA_ERRMEM`.
			*/
			int pcall(int nargs, std::optional<int> nres = std::nullopt);
			/**
			 * @brief Load %Lua code from input stream.
			 *
			 * This function will read input stream until it isn't `.good()`
			 * and feed it to `lua_load` function.
			 *
			 * @note No code will be executed, only load is done.
			 * @note If failed stream is provided, %Lua will treat it as
			 * empty one and produce empty function.
			 * @warning This function will not cut out shebang as opposed
			 * to original `luaL_loadfilex`. Feel free to open an issue or pull requeste
			 * about it if it hurts you somehow.
			 *
			 * @param istr Stream to be read from. Reading will begin at current position,
			 * no need to be seekable.
			 * @param name Chunk name to be used in messages. Name beginning with `@` will
			 * make it appear as file.
			 * @param mode Optionally, mode can be set. By default, only text chunks are
			 * allowed for security reasons.
			 *
			 * @throw Lua::SyntaxError %Lua parser failed to load chunk (`LUA_ERRSYNTAX`).
			 * @throw std::bad_alloc Call resulted in `LUA_ERRMEM`.
			*/
			void load(std::istream& istr, const std::string& name = "Lua::State::load", LoadMode mode = LoadMode::TEXT);
			/**
			 * @brief Simple wrapper around State::load to load files.
			 *
			 * @note If file does not exist, empty function will be created.
			 *
			 * @param filename Path to file being loaded.
			 * @param mode Optionally, mode can be set. By default, only text chunks are
			 * allowed for security reasons.
			 *
			 * @throw Lua::SyntaxError %Lua parser failed to load chunk (`LUA_ERRSYNTAX`).
			 * @throw std::bad_alloc Call resulted in `LUA_ERRMEM`.
			*/
			void loadFile(const std::string& filename, LoadMode mode = LoadMode::TEXT);
			/**
			 * @brief Load default %Lua library.
			 *
			 * This function is required when you need specific set of libraries.
			 * Most use cases are covered by DefaultLibsPreset passed to constructor.
			 *
			 * @param libid ID of lib you need to load.
			*/
			void loadDefaultLib(DefaultLibs libid);
			/// @}

			/// @name Warning management
			/// @{

			/**
			 * @brief Set user-defined function for warning handling.
			 *
			 * This could be useful if you want to log warnings in some special way.
			 * Warnings are internally saved using `std::stringstream` and this function
			 * will be called only when all parts are recivied.
			 *
			 * @param func User warning handler function (may be lambda).
			*/
			void setWarningFunction(const std::function<void(const std::string&)>& func);
			/**
			 * @brief Reset warning handling function.
			 *
			 * Calling this function will result in setting warning hander to
			 * default lua++ implementation (which prints warnings to stdout).
			*/
			void setDefaultWarningFunction();

			/// @}

			/// @name Type management
			/// @{

			/**
			 * @brief Register type handler.
			 *
			 * Example usage:
			 * ```
			 * L.registerType(std::make_shared<MyNS::MyTypeHandler>());
			 * ```
			 * You also may reuse same type handler for many `State`s
			 * (this is done for built-in handlers with `static` variables).
			 *
			 * @param ptr (Pointer to) Type you want to register.
			 * @return Was type registered.
			 * @note If type is already registered, this function will return false immediately.
			*/
			bool registerType(const std::shared_ptr<TypeBase>& ptr);
			/**
			 * @brief Register standard type handlers.
			 *
			 * This include:
			 *
			 * |   C++ type    |    %Lua type     |       Handler       |
			 * |---------------|------------------|---------------------|
			 * | `bool`        | `boolean`        | `TypeBool`          |
			 * | `std::string` | `string`         | `TypeString`        |
			 * | C string      | `string`         | `TypeCString`       |
			 * | `Lua::Number` | `number`         | `TypeNumber`        |
			 * | `nullptr(_t)` | `nil`            | `TypeNull`          |
			 * | `void*`       | `light userdata` | `TypeLightUserdata` |
			 *
			 * @note This function is already called when using constructor with
			 * argument different from `DefaultLibsPreset::NONE` so most users
			 * dont't need it.
			*/
			void registerStandardTypes();
			/// @}

			/// @name Interaction with %Lua stack
			/// @{

			/**
			 * @brief Push one element onto %Lua stack.
			 *
			 * This function will call appropriate type handler (which may vary depending
			 * on ones you installed).
			 *
			 * @param data Your variable to be pushed.
			 * @throw Lua::Error Type handler wasn't found.
			*/
			template<typename T>
			void pushOne(const T& data) {
				using decayed = std::decay_t<const T>;
				auto& handler = getTypeHandler(typeid(decayed));
				handler->pushValue(state, std::ref(static_cast<const decayed&>(data)));
				};

			/**
			 * @brief Get one element from %Lua stack.
			 *
			 * This function will call appropriate type handler (which may vary depending
			 * on ones you installed).
			 *
			 * ## Special cases:
			 * 1. Type is `std::any`. In this case, helper function `getGeneric()` will be
			 * called to choose type fitting value the best way.
			 * 2. Requested index in invalid. In this case function will return
			 * empty object immediately without even looking for type handler.
			 *
			 * ## Examples
			 * ```
			 * auto str  = getOne<std::string>(1);          // Get string from index 1 if possible
			 * auto any  = getOne<std::any>(1);             // Get best math for value at index 1 (special case 1)
			 * auto none = getOne<std::stringstream>(9999); // Most likely will return empty object without error (special case 2)
			 * ```
			 *
			 * @param idx Index on %Lua stack.
			 * @return `std::optional` with object if it was successfuly recivied
			 * and empty `std::optional` otherwise. Mind the "Special cases".
			 * @throw Lua::Error Type handler wasn't found.
			*/
			template<typename T>
			std::optional<T> getOne(int idx) {
				if (lua_isnone(state, idx)) return {};

				if constexpr(std::is_same<T, std::any>::value) {
						return getGeneric(idx);
						}
				else {
						auto& handler = getTypeHandler(typeid(T));

						if (!handler->checkType(state, idx)) return {};

						return std::any_cast<T>(handler->getValue(state, idx));
						}
				};

			/**
			 * @brief Push any amount of values onto %Lua stack.
			 *
			 * This function is recursive template-based generalisation over pushOne().
			 * It will try to push all given arguments one-by-one (see original function docs for details).
			 *
			 * @param value,Fargs Values to be pushed.
			 * @throw Lua::Error Type handler wasn't found.
			*/
			template<typename T, typename... Targs>
			void push(T value, Targs... Fargs) {
				pushOne(value);

				if constexpr(sizeof...(Fargs) > 0) {
						return push(Fargs...);
						}
				};

			/**
			 * @brief Get any amount of values from %Lua stack.
			 *
			 * This function is recursive template-based generalisation over getOne().
			 *
			 * It will return `std::tuple` composed of types of required items (wrapped into `std::optional`).
			 * Use C++17 tuple unpacking syntax to map them to variables (see examples below)
			 *
			 * All notes about original function apply.
			 *
			 * ## Explaining `dir` argument
			 *
			 * `dir` argumnet does control direction of moving via stack:
			 *
			 * * `true` mean forward (increment index)
			 * * `false` mean backward (decrement index)
			 *
			 * Example below should make this clear
			 *
			 * ```
			 * auto [str, num] = L.get<std::string, Lua::Number>(2, true);    2 → str,  3 → num
			 * auto [str, num] = L.get<std::string, Lua::Number>(-1, false); -1 → str, -2 → num
			 * ```
			 * If it is still not clear… well, I suck at explaining this. Open an issue and I'll try to
			 * extend this part a bit.
			 *
			 * @param idx Index of first value
			 * @param dir Direction of stack unwinding (see above)
			 * @return Tuple of requested values
			*/
			template<typename T, typename... Tres>
			auto get(int idx, bool dir) {
				auto res = getOne<T>(idx);

				if constexpr(sizeof...(Tres) > 0) {
						if (dir) {
								++idx;
								}
						else {
								--idx;
								}

						return std::tuple_cat(std::make_tuple(res), get<Tres...>(idx, dir));
						}
				else {
						return std::make_tuple(res);
						}
				};

			/// @}

			/// Get underlying `lua_State*`
			operator lua_State*() { return state; };
		};

	/**
	 * @brief Class to manage State from %Lua-side call
	 *
	 * This class is a RAII wrapper around `Lua::getFromLuaState()`.
	 * When constructed, it will check if managed lua_State equal to
	 * passed one. If not (we are called from coroutine), it will be
	 * updated. Original lua_State will be restored on destruction, so
	 * you don't have to worry about exceptions/return.
	*/
	class StatePtr {
		protected:
			State* ptr = nullptr; ///< Pointer to State used for current lua_State
			lua_State* oldState = nullptr; ///< lua_State that was managed before this call (`nullptr` if not changed)

		public:
			StatePtr() = delete;
			StatePtr(lua_State* L); ///< Construct object and update lua_State in recivied State if required
			virtual ~StatePtr(); ///< Destructor

			/// Access State object as normal
			State* operator* () { return ptr; };
			/// Access State object as normal
			State* operator->() { return ptr; };
		};
	};
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
