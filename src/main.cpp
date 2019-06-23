#include <iostream>
#include <fstream>
#include <string>
#include "lua++/State.hpp"
#include "lua++/TypeHelper.hpp"
#include "lua++/Error.hpp"
#include <assert.h>

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
		std::string MethodWrapped(lua_Number Ln, std::string str) {
			std::cout << "Numbery: " << Ln << std::endl;
			std::cout << "Wordy: " << str << std::endl;
			return "Yeah";
			};
		void MethodWrappedVoid() { std::cout << "I take nothing and give nothing" << std::endl; }
		std::tuple<std::string, Lua::Number> MethodManyRes() { return {"Meaning of life is", 42_li}; };
		int __index(Lua::StatePtr& Lp) { Lp->push("Unknown index"); return 1; };
	};

class EmptyClass {};

const Lua::FunctionsTable MyTestClass::metamethods = {
		{"__call", [](Lua::StatePtr & Lp) { Lp->push("Who called me? ^_^"); return 1; }}
	};

const Lua::MethodsTable<MyTestClass> MyTestClass::methods = {
	Lua::CppMethodPair("TestMethod", &MyTestClass::TestMethod),
	Lua::CppMethodPair("TestMethod2", &MyTestClass::TestMethod2),
		{"MethodWrapped", Lua::CppMethodWrapper<MyTestClass, Lua::Number, std::string>(&MyTestClass::MethodWrapped)},
		{"MethodWrappedVoid", Lua::CppMethodWrapper<MyTestClass>(&MyTestClass::MethodWrappedVoid)},
		{"MethodManyRes", Lua::CppMethodWrapper<MyTestClass>(&MyTestClass::MethodManyRes)},
	};

int main() {
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
		std::cout << "Nothing to show you here: " << *ret << std::endl;
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
		L.push("I wish my dream #", 1_li, " will come ", true);
		auto [res1, res2, res3, res4] = L.get<std::string, Lua::Number, std::any, bool>(2, true);
		std::cout << *res1 << (lua_Integer)*res2 << std::any_cast<std::string>(*res3) << *res4 << std::endl;
		auto rescnt = L.pcall(4);
		std::cout << "We got " << rescnt << " results (from 0)" << std::endl;
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
print(testObj())

local ovar
do
	local <toclose> var = testObjStatic() -- Shouldn't cause warnings
	ovar = var
end
print(pcall(ovar)) -- NOTE: Doesn't fail. You must check that value is valid.

print(testObj:MethodWrapped(10, "Hey! Listen!"))
testObj:MethodWrappedVoid()
print(testObj:MethodManyRes())
		)LUA"
		);
		auto Lp = Lua::StatePtr(L);
		assert(helper->pushStatic(Lp));
		L.pcall(1, 0);
		}

	return EXIT_SUCCESS;
	}
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
