#include "lua++/State.hpp"
#include "lua++/Error.hpp"
#include "lua++/CppFunction.hpp"

#include <array>
#include <iostream>
#include <fstream>

static void defaultWarningsHandler(const std::string& warn) {
	std::cout << "[Lua warning]: " << warn << std::endl;
	};

namespace Lua {
	namespace {
		/**
		 * @brief Helper class for loading from C++ stream.
		*/
		class StreamReadHelper {
			private:
				std::istream& stream;         ///< Source stream.
				int n = 0;                    ///< Number of pre-read characters.
				std::array<char, BUFSIZ> buff {}; ///< Internall buffer.

				/**
				 * @brief Prepare object to usage
				 * @warning Currently, it's no-op.
				 * If required, I can add shebang skipper as in original function.
				*/
				void prepare() {

					};
			public:
				StreamReadHelper(std::istream& s): stream(s) { prepare(); }; ///< Main constructor
				/**
				 * @brief Read data from stream in %Lua style.
				**/
				const char* luaRead(size_t* size) {
					if (!stream.good()) return nullptr;

					stream.read(buff.data(), BUFSIZ);
					*size = stream.gcount();
					return buff.data();
					};
				/**
				 * @brief Forward raw call to C++ object.
				*/
				static const char* luaReadStatic([[maybe_unused]] lua_State* L, void* ud, size_t* size) {
					return static_cast<StreamReadHelper*>(ud)->luaRead(size);
					};
			};

		/**
		 * @brief Helper class for loading from C++ string.
		*/
		class StringReadHelper {
			private:
				std::string_view str; ///< Source stream.
				bool read = false;    ///< Was string read.

			public:
				StringReadHelper(std::string_view s): str(s) {}; ///< Main constructor
				/**
				 * @brief Read data from string in %Lua style.
				**/
				const char* luaRead(size_t* size) {
					if (read) return nullptr;

					read = true;
					*size = str.length();
					return str.data();
					};
				/**
				 * @brief Forward raw call to C++ object.
				*/
				static const char* luaReadStatic([[maybe_unused]] lua_State* L, void* ud, size_t* size) {
					return static_cast<StringReadHelper*>(ud)->luaRead(size);
					};
			};

		const std::unordered_map<DefaultLibs, luaL_Reg> luaLibs = {
			// Base
				{DefaultLibs::BASE,      {LUA_GNAME, luaopen_base}},
			// Safe
				{DefaultLibs::COROUTINE, {LUA_COLIBNAME, luaopen_coroutine}},
				{DefaultLibs::TABLE,     {LUA_TABLIBNAME, luaopen_table}},
				{DefaultLibs::STRING,    {LUA_STRLIBNAME, luaopen_string}},
				{DefaultLibs::MATH,      {LUA_MATHLIBNAME, luaopen_math}},
				{DefaultLibs::UTF8,      {LUA_UTF8LIBNAME, luaopen_utf8}},
			// Questionable safety
				{DefaultLibs::PACKAGE,   {LUA_LOADLIBNAME, luaopen_package}},
			// Unsafe
				{DefaultLibs::IO,        {LUA_IOLIBNAME, luaopen_io}},
				{DefaultLibs::OS,        {LUA_OSLIBNAME, luaopen_os}},
				{DefaultLibs::DEBUG,     {LUA_DBLIBNAME, luaopen_debug}}
			};

		const std::unordered_map<LoadMode, const char*> luaLoadModes = {
				{LoadMode::TEXT,   "t" },
				{LoadMode::BINARY, "b" },
				{LoadMode::BOTH,   "bt"}
			};

		int luaPanic(lua_State* L) {
			std::cerr << "Lua panic (unprotected call): " << lua_tostring(L, -1) << std::endl;
			return 0;
			};

		void* luaAlloc([[maybe_unused]] void* ud, void* ptr, [[maybe_unused]] size_t oldsize, size_t newsize) {
			if (newsize == 0) {
					std::free(ptr);
					return nullptr;
					}
			else {
					return std::realloc(ptr, newsize);
					}
			};
		};

	void State::warnHandler(void* ud, const char* msg, int tocont) {
		auto L = static_cast<State*>(ud);
		L->warnBuf << msg;

		if (!tocont) {
				L->warnFunc(L->warnBuf.str());
				L->warnBuf.str("");
				}
		};

	void State::updateStatePointer() {
		*luaStatePtr = this;
		lua_setwarnf(mainState, warnHandler, this);
		};

	const std::shared_ptr<TypeBase>& State::getTypeHandler(const std::type_info& tinfo) const {
		std::type_index type(tinfo);

		if (!knownTypes.count(type)) {
				throw Lua::Error("Missing type handler");
				}

		return knownTypes.at(type);
		};

	void State::throwLuaError() {
		auto msg = getOne<std::string>(-1);
		pop(1);

		if (msg) {
				throw StateError("[Lua error]: " + *msg);
				}
		else {
				throw StateError("[Lua error]: non-string error");
				}
		};

	State::State(DefaultLibsPreset openlibs, lua_Alloc alloc, void* ud) {
		if (!alloc) {
				alloc = luaAlloc;
				}

		mainState = (state = lua_newstate(alloc, ud));

		if (!mainState) throw Lua::Error("Can't create state");

		lua_atpanic(mainState, &luaPanic);

		luaStatePtr = new State*(this);
		static_assert(sizeof(State***) <= LUA_EXTRASPACE);
		auto ptr = static_cast<State*** >(lua_getextraspace(state));
		*ptr = luaStatePtr;
		updateStatePointer();

		setDefaultWarningFunction();

		if (openlibs != DefaultLibsPreset::NONE) {
				loadDefaultLib(DefaultLibs::BASE);
				registerStandardTypes();

				switch (openlibs) {
					case DefaultLibsPreset::ALL:
						loadDefaultLib(DefaultLibs::IO);
						loadDefaultLib(DefaultLibs::OS);
						loadDefaultLib(DefaultLibs::DEBUG);

						[[fallthrough]];

					case DefaultLibsPreset::SAFE_WITH_PACKAGE:
						[[fallthrough]];

					case DefaultLibsPreset::SAFE_WITH_STRIPPED_PACKAGE:
						loadDefaultLib(DefaultLibs::PACKAGE);

						[[fallthrough]];

					case DefaultLibsPreset::SAFE:
						loadDefaultLib(DefaultLibs::COROUTINE);
						loadDefaultLib(DefaultLibs::TABLE);
						loadDefaultLib(DefaultLibs::STRING);
						loadDefaultLib(DefaultLibs::MATH);
						loadDefaultLib(DefaultLibs::UTF8);

						[[fallthrough]];

					case DefaultLibsPreset::BASE:
						// We already loaded it first
						[[fallthrough]];

					case DefaultLibsPreset::NONE:
						// Nothing to load
						;
						}

				if (openlibs == DefaultLibsPreset::SAFE_WITH_STRIPPED_PACKAGE)
					stripPackageLibrary();
				}
		};

	bool State::loadPackageTables() {
		luaL_checkstack(state, 2, nullptr);
		// Stack: xxx
		luaL_getsubtable(state, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
		// Stack: xxx, package.loaded
		lua_getfield(state, -1, luaLibs.at(DefaultLibs::PACKAGE).name);

		// Stack: xxx, package.loaded, package
		if (!lua_toboolean(state, -1)) {
				// Package library not loaded, fail gracefully
				pop(2);
				// Stack: xxx
				return false;
				}

		return true;
		}

	bool State::stripPackageLibrary() {
		// Stack: xxx
		if (!loadPackageTables()) return false;

		// Stack: xxx, package.loaded, package

		// Remove filed from table on top of stack
		auto removeField = [this](const char* name) {
			push(nullptr);
			lua_setfield(state, -2, name);
			};

		// Stack: xxx, package.loaded, package
		// Remove library functions
		removeField("loadlib"); // Allow access to C code
		removeField("searchpath"); // Allow scanning filesystem

		// Stack: xxx, package.loaded, package
		lua_getfield(state, -1, "searchers");

		// Stack: xxx, package.loaded, package, package.searchers
		if (lua_toboolean(state, -1)) {
				// Remove all searchers but first one
				for (auto i = luaL_len(state, -1); i >= 2; --i) {
						// Stack: xxx, package.loaded, package, package.searchers
						push(nullptr);
						lua_rawseti(state, -2, i);
						}
				}

		// Stack: xxx, package.loaded, package, package.searchers
		pop(3);
		// Stack: xxx
		return true;
		};

	bool State::addSearcher(SearcherFunction& loader) {
		CppFunction LuaSide = [loader](StatePtr & ptr) -> int {
			auto name = ptr->getOne<std::string>(2);
			std::optional<CppFunction> func;
			std::optional<std::string> data;

			try {
					std::tie(func, data) = loader(name.value());
					std::optional<CppFunctionWrapper> funcWrapped;

					if (func)
						funcWrapped.emplace(*func);

					return ptr->push(funcWrapped, data);
					}
			catch (const std::exception& e) {
					lua_warning(**ptr, "exception in C++ searcher (typeid ", false);
					lua_warning(**ptr, typeid(e).name(), false);
					lua_warning(**ptr, "): ", false);
					lua_warning(**ptr, e.what(), true);
					return 0;
					}
			};


		// Stack: xxx
		if (!loadPackageTables()) return false;

		// Stack: xxx, package.loaded, package
		luaL_checkstack(state, 2, nullptr);
		lua_getfield(state, -1, "searchers");
		// Stack: xxx, package.loaded, package, package.searchers

		if (!lua_toboolean(state, -1)) {
				pop(3);
				return false;
				}

		// Stack: xxx, package.loaded, package, package.searchers
		pushOne(LuaSide);
		// Stack: xxx, package.loaded, package, package.searchers, our searcher
		lua_rawseti(state, -2, luaL_len(state, -2));
		// Stack: xxx, package.loaded, package, package.searchers
		pop(3);
		return true;
		};

	bool State::addPreloaded(const std::string& name, const CppFunction& loader) {
		// Stack: xxx
		if (!loadPackageTables()) return false;

		// Stack: xxx, package.loaded, package
		luaL_checkstack(state, 1, nullptr);
		lua_getfield(state, -1, "preload");
		// Stack: xxx, package.loaded, package, package.preload

		if (!lua_toboolean(state, -1)) {
				pop(3);
				return false;
				}

		// Stack: xxx, package.loaded, package, package.preload
		push(name, CppFunctionWrapper(loader));
		// Stack: xxx, package.loaded, package, package.preload, name, loader
		lua_rawset(state, -3);
		// Stack: xxx, package.loaded, package, package.preload
		pop(3);
		return true;
		};

	bool State::addPreloaded(const std::string& name, lua_CFunction loader) {
		// Stack: xxx
		if (!loadPackageTables()) return false;

		// Stack: xxx, package.loaded, package
		luaL_checkstack(state, 1, nullptr);
		lua_getfield(state, -1, "preload");
		// Stack: xxx, package.loaded, package, package.preload

		if (!lua_toboolean(state, -1)) {
				pop(3);
				return false;
				}

		// Stack: xxx, package.loaded, package, package.preload
		push(name);
		luaL_checkstack(state, 1, nullptr);
		lua_pushcfunction(state, loader);
		// Stack: xxx, package.loaded, package, package.preload, name, loader
		lua_rawset(state, -3);
		// Stack: xxx, package.loaded, package, package.preload
		pop(3);
		return true;
		};

	State::~State() {
		if (mainState) {
				lua_close(mainState);
				delete luaStatePtr;
				}
		};

	int State::pcall(int nargs, std::optional<int> nres) {
		// Stack: xxx + function + nargs
		int oldStacktop = lua_gettop(state);
		auto res = lua_pcall(state, nargs, nres.value_or(LUA_MULTRET), 0);

		if (res == LUA_OK)     {
				// Stack: xxx + nres
				return lua_gettop(state) - oldStacktop + 1 + nargs;
				}
		else if (res == LUA_ERRRUN) {
				// Stack: xxx + errmsg
				throwLuaError();
				}
		else if (res == LUA_ERRMEM) {
				throw std::bad_alloc(); // Uh-oh
				}
		else                        {
				std::abort();
				}
		};

	template<typename T>
	void State::loadInternal(T& reader, const std::string& name, LoadMode mode) {
		auto res = lua_load(state, T::luaReadStatic, &reader, name.c_str(), luaLoadModes.at(mode));

		if (res == LUA_OK) {
				return; // Successful load
				}
		else if (res == LUA_ERRSYNTAX) {
				auto err = getOne<std::string>(-1); // Must not fail
				throw SyntaxError(err.value());
				}
		else if (res == LUA_ERRMEM) {
				throw std::bad_alloc();
				}
		else std::abort(); // I hope you have your towel ready, things are going really messy around there…
		};

	void State::load(std::istream& istr, const std::string& name, LoadMode mode) {
		StreamReadHelper reader(istr);
		loadInternal(reader, name, mode);
		};

	void State::load(const std::string& str, LoadMode mode) {
		StringReadHelper reader(str);
		loadInternal(reader, str, mode);
		};

	void State::loadFile(const std::string& filename, LoadMode mode) {
		std::ifstream fin(filename);
		load(fin, "@" + filename, mode);
		};

	void State::loadDefaultLib(DefaultLibs libid) {
		const auto& lib = luaLibs.at(libid); // Must never fail
		luaL_requiref(state, lib.name, lib.func, 1);
		pop(1);  /* remove lib */
		};

	void State::setWarningFunction(const std::function<void(const std::string&)>& func) {
		warnFunc = func;
		};

	void State::setDefaultWarningFunction() {
		setWarningFunction(defaultWarningsHandler);
		};

	bool State::registerType(const std::shared_ptr<TypeBase>& ptr) {
		std::type_index type(ptr->getType());

		if (knownTypes.count(type)) {
				return false;
				}

		knownTypes[type] = ptr;
		knownTypesList.push_back(ptr);
		ptr->init(*this);
		return true;
		};

	void State::registerStandardTypes() {
		// Same object is used for every instance
		static const std::array<std::shared_ptr<TypeBase>, 8> dtypes = {
			std::make_shared<Lua::TypeBool>(),
			std::make_shared<Lua::TypeString>(),
			std::make_shared<Lua::TypeCString>(),
			std::make_shared<Lua::TypeNumber>(),
			std::make_shared<Lua::TypeNull>(),
			std::make_shared<Lua::TypeLightUserdata>(),
			std::make_shared<Lua::TypeCppFunction>(),
			std::make_shared<Lua::TypeCppFunctionWrapper>()
			};

		for (const auto& elem : dtypes) {
				registerType(elem);
				}
		};

	std::any State::getGeneric(int idx) {
		for (const auto& type : knownTypesList) {
				if (type->isBestType(state, idx)) {
						return std::any(type->getValue(state, idx));
						}
				}

		return std::any();
		};

	StatePtr::StatePtr(lua_State* L) {
		ptr = State::getFromLuaState(L);

		if (ptr->state != L) {
				oldState = ptr->state;
				ptr->state = L;
				}
		};

	StatePtr::StatePtr(State& L) {
		ptr = &L;
		};

	StatePtr::~StatePtr() {
		if (oldState) {
				ptr->state = oldState;
				}
		}
	}
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
