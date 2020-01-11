#include <iostream>
#include <fstream>
#include <string>
#include "lua++/State.hpp"
#include "lua++/TypeHelper.hpp"
#include "lua++/Error.hpp"
#include <assert.h>

// Set to `true` to run test code 6000 times (for profiling of unobvious bugs)
constexpr bool doProfiling = false;

using namespace std::string_literals; // for s
using namespace Lua::NumberLiterals;  // for _ln and _li

#define printtype(T) std::cout << typeid(T).name() << std::endl

class MyTestClass {
	public:
		MyTestClass() {};
		MyTestClass(Lua::StatePtr&) { std::cout << "Lua constructor here" << std::endl; };
		~MyTestClass() { std::cout << "Nap time" << std::endl; };
		static const Lua::FunctionsTable metamethods;
		static const Lua::MethodsTable<MyTestClass> methods;

		int TestMethod(Lua::StatePtr& Lp) { std::cout << "I'm flying!" << std::endl; return 0; };
		int TestMethod2(Lua::StatePtr& Lp) { std::cout << "Please put me down." << std::endl; return 0; };
		std::string MethodWrapped(lua_Number Ln, const std::string& str) {
			std::cout << "Numbery: " << Ln << std::endl;
			std::cout << "Wordy: " << str << std::endl;
			return "Yeah";
			};
		void MethodWrappedVoid() { std::cout << "I take nothing and give nothing" << std::endl; }

		void MethodWrappedOverloaded() { std::cout << "Overload #1" << std::endl; }
		void MethodWrappedOverloaded(const std::string&) { std::cout << "Overload #2" << std::endl; }

		std::tuple<std::string, Lua::Number> MethodManyRes() { return {"Meaning of life is", 42_li}; };
		int __index(Lua::StatePtr& Lp) { Lp->push("Unknown index"); return 1; };
	};

class EmptyClass {};

const Lua::FunctionsTable MyTestClass::metamethods = {
	//{"__call", [](Lua::StatePtr & Lp) { Lp->push("Who called me? ^_^"); return 1; }}
		{
		"__call", Lua::CppFunctionNative<std::shared_ptr<MyTestClass>, std::string, std::any>([](auto, const std::string & str, const std::any&) -> const char* {
			std::cout << "I'm a free function who got string: " << str << std::endl;
			return "Glorious freedom!";
			})
		}
	};

const Lua::MethodsTable<MyTestClass> MyTestClass::methods = {
	Lua::CppMethodPair("TestMethod", &MyTestClass::TestMethod),
	Lua::CppMethodPair("TestMethod2", &MyTestClass::TestMethod2),
	Lua::CppMethodNativePair<Lua::Number, std::string>("MethodWrapped", &MyTestClass::MethodWrapped),
	Lua::CppMethodNativePair<>("MethodWrappedVoid", &MyTestClass::MethodWrappedVoid),
	Lua::CppMethodNativePair<>("MethodManyRes", &MyTestClass::MethodManyRes),
	Lua::CppMethodNativePair<>("MethodWrappedOverloaded1", static_cast<void(MyTestClass::*)(void)>(&MyTestClass::MethodWrappedOverloaded)),
	Lua::CppMethodNativePair<std::string>("MethodWrappedOverloaded2", static_cast<void(MyTestClass::*)(const std::string&)>(&MyTestClass::MethodWrappedOverloaded)),
	};

// Simple "echo" function
// To demonstrate that argument order is same as results
// (I thought that it's reverse for some reason)
int echoFunc(Lua::StatePtr& Lp) {
	return lua_gettop(**Lp) - 1;
	};

void testBasic() {
	Lua::State state(Lua::DefaultLibsPreset::SAFE_WITH_PACKAGE);
	//Lua::State stateA = state;
	std::cout << "Hi" << std::endl;

	// Test strings
	lua_getglobal(state, "print");
	state.pushOne("Hello\0 from"s);
	state.pushOne("Lua side!\0 Nope!");
	lua_call(state, 2, 0);

	// Stack is empty

	auto L = std::move(state);
	std::cout << "Hi :)" << std::endl;

		{
		L.pushOne("Just testing some stuff\0 Umm, you aren't reading this in output, right?");
		auto ret = L.getOne<std::string>(-1);
		std::cout << *ret << std::endl;
		lua_pop(L, 1);
		}

	// Stack is empty
	// Testing numbers

		{
		// Normal numbers (always use literals)
		L.pushOne(5.4_ln);
		auto ret = L.getOne<Lua::Number>(-1);
		std::cout << "I've got a lua_Number for you: " << (lua_Number)*ret << std::endl;
		lua_pop(L, 1);
		}
	// Stack is empty
		{
		// And integers (use literals as well)
		L.pushOne(42_li);
		auto ret = L.getOne<Lua::Number>(-1);
		std::cout << "Oh, and lua_Integer: " << (lua_Integer)*ret << std::endl;
		lua_pop(L, 1);
		}
	// Stack is empty
		{
		// Negative integer
		L.pushOne(-67_li);
		auto ret = L.getOne<Lua::Number>(-1);
		std::cout << "And anoter one: " << (lua_Integer)*ret << std::endl;
		lua_pop(L, 1);
		}

	// Stack is empty
	// Testing nil/nullptr_t

		{
		L.pushOne(nullptr);
		auto ret = L.getOne<std::nullptr_t>(-1);
#if defined(__GNUG__) && !defined(__clang__)
		std::cout << "Nothing to show you here: " << (void*)*ret << std::endl; // GCC can't print it for me
#else
		std::cout << "Nothing to show you here: " << *ret << std::endl;
#endif
		lua_pop(L, 1);
		}

	// Stack is empty
	// Testing void*

		{
		L.pushOne((void*)L);
		auto ret = L.getOne<void*>(-1);
		std::cout << "Just a ptr: " << *ret << std::endl;
		lua_pop(L, 1);
		}

	// Stack is empty
	// Testing bool and pointer from state

		{
		auto x = Lua::StatePtr((lua_State*)L);
		x->pushOne(false);
		auto ret = x->getOne<bool>(-1);
		std::cout << "This sentence is " << std::boolalpha << *ret << std::endl;
		lua_pop(L, 1);
		}

	// Stack is empty
	// Templates magic time!

		{
		lua_getglobal(L, "print");
		L.push("I wish my dream #", 1_li);
		L.push(std::make_tuple(" will come ", true));
		auto [res1, res2, res3, res4] = L.get<std::string, Lua::Number, std::any, bool>(2, true);
		std::cout << *res1 << (lua_Integer)*res2 << std::any_cast<std::string>(*res3) << *res4 << std::endl;
		auto rescnt = L.pcall(4);
		std::cout << "We got " << rescnt << " results (from 0)" << std::endl;
		}

	// Stack is empty
	// Testing optionals

		{
		auto a = std::make_optional<std::string>("Before nothing");
		std::optional<std::string> b;
		auto c = std::make_optional<std::string>("After nothing");

		lua_getglobal(L, "print");
		L.push(a, b, c);
		auto [a1, b1, c1] = L.get<std::string, std::string, std::string>(2, true);
		std::cout << a1.value_or("nothing") << " " << b1.value_or("nothing") << " " << c1.value_or("nothing") << std::endl;
		L.pcall(3);
		}

	// Stack is empty
	// Test warnings

		{
		lua_warning(L, "This is", true);
		lua_warning(L, " a ", true);
		lua_warning(L, "warning", false);
		lua_warning(L, "Another one", false);
		L.setWarningFunction([](const std::string&) {
			std::cout << "Hi! ^_^ I'm custom warning handler!" << std::endl;
			});
		lua_warning(L, "You souldn't ", true);
		lua_warning(L, "see this", false);
		L.setDefaultWarningFunction();
		lua_warning(L, "Aaaand, we're back.", false);
		}

	// Stack is empty
	// Test errors

		{
		lua_getglobal(L, "error");
		L.push("checking lua-side error handling");

		try {
				L.pcall(1);
				}
		catch (const Lua::StateError& err) {
				std::cout << "We got an error: " << err.what() << std::endl;
				}
		}
		{
		lua_getglobal(L, "error");
		L.push(true);

		try {
				L.pcall(1);
				}
		catch (const Lua::StateError& err) {
				std::cout << "We got an error: " << err.what() << std::endl;
				}
		}

	// Stack is empty
	// Test load

		{
		auto runStream = [&L](std::istream & stream, const std::string & name) {
			try {
					L.load(stream, name);
					L.pcall(0, 0);
					}
			catch (const Lua::SyntaxError& err) {
					std::cout << "We got a syntax error in " << name << ": " << err.what() << std::endl;
					}
			catch (const Lua::StateError& err) {
					std::cout << "We got a runtime error in " << name << ": " << err.what() << std::endl;
					}
			catch (const std::exception& err) {
					std::cout << "We got a non-lua error (somehow) in " << name << ": " << err.what() << std::endl;
					}
			};

		auto runFile = [&runStream](const std::string & name) {
			std::ifstream fin(name);
			runStream(fin, "@" + name);
			};

		runFile("test_normal.lua");
		runFile("syntax_error.lua");
		runFile("i_do_not_exsist.lua");
			{
			// Test loading of long chunks
			std::stringstream sstr;
			sstr << "local str = [[";

			for (size_t i = 0; i < BUFSIZ * 4; ++i) {
					sstr << "A";
					}

			sstr << "]]" << std::endl
				<< "print([[Scream lenght: ]], #str)";

			runStream(sstr, "std::stringstream");
			}
		//runStream(std::cin, "@stdin"); // To run user input. It works!
		L.load(
			R"LUA(print([[I can insert Lua code into C++ like this! // Even with comments!
Isn't it awesome? ♥]]))LUA"
		);
		L.pcall(0, 0);
		}

	// Stack is empty
	// Test C++ types

		{
		L.push((Lua::CppFunction)echoFunc);
		lua_setglobal(L, "echoFunc");
		L.registerType(std::make_shared<Lua::TypeHelper<EmptyClass>>()); // Check that empty class is fine
		auto helper = std::make_shared<Lua::TypeHelper<MyTestClass>>();
		L.registerType(helper);
		//L.loadDefaultLib(Lua::DefaultLibs::DEBUG);
		L.load(
			R"LUA(
local testObjStatic = ({...})[1]
local testObj = testObjStatic()
print(testObj)
print(testObj.TestMethod)
print(CppFunctionWrapper(testObj.TestMethod))
print(testObj.HelloWorld)
testObj:TestMethod()
CppFunctionWrapper(testObj.TestMethod2, testObj.TestMethod)(testObj)
print(testObj("SPAAAAAAAAAAAAAAAAAAAAAAAACE!", "Optional arg. Don't mind me.", "I'm hidding there."))

local ovar
do
	local var <close> = testObjStatic() -- Shouldn't cause warnings
	ovar = var
	print(pcall(ovar, "some args"))
end
print(pcall(ovar, "some args")) -- NOTE: Does fail because CppFunctionNative is used.

print(testObj:MethodWrapped(10, "Hey! Listen!"))
print(pcall(testObj.MethodWrapped))
testObj:MethodWrappedVoid()
print(testObj:MethodManyRes())
print("Doing test:", echoFunc(1,2,3,4))
		)LUA"
		);
		auto Lp = Lua::StatePtr(L);
		helper->pushStatic(Lp);
		L.pcall(1, 0);
		}
	}

int cloader(lua_State* L) {
	lua_pushstring(L, "C loader signature loader working");
	return 1;
	};

void testPackage() {
	auto cpploader = [](Lua::StatePtr & Lp) -> int {
		Lp->push("C++ signature loader working");
		return 1;
		};
		{
		std::cout << "Testing loading addPreload" << std::endl;
		Lua::State L(Lua::DefaultLibsPreset::SAFE_WITH_STRIPPED_PACKAGE);
		L.addPreloaded("cloader", cloader);
		L.addPreloaded("cpploader", cpploader);
		L.load(
			R"LUA(
print(package.preload.cloader)
print(pcall(require, 'cloader'))
print(package.preload.cpploader)
print(pcall(require, 'cpploader'))
		  )LUA"
		);
		L.pcall(0);
		}
		{
		Lua::SearcherFunction searcher = [&cpploader](const std::string & name) -> std::tuple<std::optional<Lua::CppFunction>, std::optional<std::string>> {
			if (name == "cpploader") {
					return {cpploader, "Custom loader data"};
					}

			return {std::nullopt, "Custom searcher lookup failed"};
			};
		Lua::State L(Lua::DefaultLibsPreset::SAFE_WITH_STRIPPED_PACKAGE);
		L.addSearcher(searcher);
		L.load(
			R"LUA(
print(pcall(require, 'cpploader'))
print(pcall(require, 'i_do_not_exsist'))
		  )LUA"
		);
		L.pcall(0);
		}
	};

int luaopen_lpeg(lua_State* L);

void testLpeg() {
	Lua::State L(Lua::DefaultLibsPreset::SAFE_WITH_STRIPPED_PACKAGE);
	L.addPreloaded("lpeg", luaopen_lpeg);
	L.load(
		R"LUA(
print(pcall(require, 'lpeg'))
-- Use lpeg as usual here, I don't know it
		  )LUA"
	);
	L.pcall(0);
	};

void myStuff() {
	Lua::State state(Lua::DefaultLibsPreset::SAFE_WITH_STRIPPED_PACKAGE);
	state.load("print(require 'cmath');");
	state.pcall(0, 0);
	}

void doTests() {
	testBasic();
	testPackage();
	testLpeg();
	};

int main(int argc, const char* argv[]) {
	/*
	 * Profiling on my PC (6000 runs, output redirected to `/dev/null`):
	 * Clang 8.0.0:
	 * Debug:   ~8.8s = 681 run/s
	 * Release: ~4.8-4.9s = 1250 run/s
	 * GCC 8.3.0:
	 * Debug:   ~8.5s = 705 run/s
	 * Release: ~4.8-4.9s = 1250 run/s
	 *
	 * NOTE: tests must be done in `test_files` directory to do them all
	*/
	try {
			if constexpr(doProfiling) {
					for (size_t i = 0; i < 6000; ++i)
						doTests();
					}
			else {
					doTests();
					//myStuff();
					}
			}
	catch (const std::exception& e) {
			std::cerr << "Exception caught! what(): " << e.what() << std::endl;
			return EXIT_FAILURE;
			}

	std::cerr << "Tests done fine. Ex(c)iting." << std::endl;
	return EXIT_SUCCESS;
	};

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
