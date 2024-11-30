# Goal

The goal of the namespace TODO item is to move all Rebirth symbols into one of two top level namespaces: `dcx` for common symbols and `dsx` for similar symbols.  Code is considered common if it is compiled to the same output independent of which game is built.  Code is considered similar if it is not common.

To be common, code must not:

* Examine the preprocessor symbol `DXX_BUILD_DESCENT`:
```c++
int f()
{
	// not common because the return statement varies by game
#if DXX_BUILD_DESCENT == 1
	return 1;
#elif DXX_BUILD_DESCENT == 2
	return 4;
#endif
}
```
* Use any macro which is defined inside a game-specific preprocessor directive:
```c++
#if DXX_BUILD_DESCENT == 1
#define MY_MACRO	2
#elif DXX_BUILD_DESCENT == 2
#define MY_MACRO	4
#endif

int f()
{
	return MY_MACRO;	// not common, because MY_MACRO varies by game
}
```
* Use any type which is not common:
```c++
struct A
{
	int a;
#if DXX_BUILD_DESCENT == 1
	int b;
#endif
};

int f(A &a)
{
	return a.b;	// not common, because struct A is not common
}
```
* Call any function which is not common:
```c++
int f()
{
#if DXX_BUILD_DESCENT == 1
	return 1;
#elif DXX_BUILD_DESCENT == 2
	return 4;
#endif
}

int g()
{
	// not common because f() varies by game
	return f() + 1;
}
```

To be common, data variables must:
* Be an instance of a type which is common: a built-in type or a UDT which is common.
* Exist in both games.  If a data variable exists in one game and not in the other, then that variable is not common, even if it is a type which is common.  For example, afterburner charge is a built-in type (`int`), but is not common because it is not defined in Descent 1.
* Be the same type in both games.

To be common, user defined types must not:
* Examine the preprocessor symbol `DXX_BUILD_DESCENT`.
* Depend on any macro which is defined inside a game-specific preprocessor directive:
```c++
#if DXX_BUILD_DESCENT == 1
#define MY_MACRO	2
#elif DXX_BUILD_DESCENT == 2
#define MY_MACRO	4
#endif
struct A
{
	int a[MY_MACRO];	// not common because a[] depends on MY_MACRO
};
```
* Use non-common UDTs as members or base classes:
```c++
struct A
{
	int a;
#if DXX_BUILD_DESCENT == 1
	int b;
#endif
};

struct B : public A	// not common because base class A is not common
{
	int c;
};

struct C // not common because member `a` is not common
{
	A a;
	int d;
};
```

# Implementation
Anyone may work on this TODO item.  Symbols in the global namespace should be moved to `namespace dcx` if the symbol is common and to `namespace dsx` if the symbol is not common.  Currently, `dxxsconf.h` defines `dsx` as a macro that expands to `d1x` or `d2x`, as appropriate.

A symbol declared in a namespace is distinct from a symbol of the same name in a different namespace.  `SConstruct` enables the compiler warning `-Wmissing-declarations`, so moving a function declaration into a namespace will provoke a warning that the definition is missing a declaration.  Moving only the definition into a namespace will also provoke the warning.  In both cases, this is a reminder from the compiler that both the declaration and the definition must be moved into the namespace.  Without this warning, the build would fail when the linker searches for the symbol in one namespace, but it is only present in the other namespace.

Moving a data variable declaration or definition into a namespace will **not** provoke a compiler warning.  However, it will still produce a linker error if the variable is declared `extern` in one namespace, but defined only in a different namespace.

As a transition measure, `dxxsconf.h` includes `using namespace dcx;` and, when `dsx` is enabled, `using namespace dsx;`.  These transition assistants may mask mistakes where a symbol is placed in the wrong namespace.  These assistants will be removed before this TODO item is considered complete.

As a developer aid, `dxxsconf.h` declares classes named `dcx` and `dsx` in various scopes chosen to conflict with erroneous namespace usage.  Namespaces `dcx` and `dsx` should only ever be declared at the top level.  Neither should ever appear inside an anonymous namespace, nor inside itself, nor inside the other.  The classes declared in `dxxsconf.h` will force an error in those cases.

Files should never be included while a namespace is open.  This is wrong:
```c++
namespace dcx {
int a;
#include "other.h"	// wrong, other.h not expecting to be inside a namespace
int b;
}
```
Namespaces can be closed and reopened.  There are two ways to rewrite the above to be correct.  First:
```c++
namespace dcx {
int a;
}
#include "other.h"
namespace dcx {
int b;
}
```
Second:
```c++
namespace dcx {
int a;
int b;
}
#include "other.h"
```
The second form will be used eventually, but the first is more convenient during the transition phase.
