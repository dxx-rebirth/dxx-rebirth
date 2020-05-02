DXX-Rebirth requires a compiler that implements the C++17 standard.  A fully conforming compiler is recommended, but some omissions can be handled by SConf tests that enable a fallback to emulate the feature.

# Required C++11 features
DXX-Rebirth code uses C++11 and C++14 features present in >=clang-9.0 and >=gcc-7.5.  Some of these features are probed in the SConf tests so that an error can be reported if the feature is missing.  However, since these are considered the minimum supported compiler versions, and existing SConf tests reject older compilers, some C++14 features that are new in gcc-7.5 may be used without a corresponding test in SConf.

These C++11 features are required to build DXX-Rebirth:

* [Rvalue references][cppr:cpp/language/reference]
(`void a(A &&);`)
* [Variadic templates][cppr:cpp/language/parameter_pack]
(`template <typename... T> class A{};`)
* [`auto`-typed variables][cppr:cpp/language/auto]
(`auto a = f();`)
    * Trailing function return type (`auto f() -> int;`)
* [Lambdas][cppr:cpp/language/lambda]
(`f([]{ return 1; });`)
* [`decltype()`][cppr:cpp/language/decltype]
(`typedef decltype(a()) b;`)
* [Alias templates][cppr:cpp/language/type_alias]
(`using A = B;
template <typename T> using C = D<T, T>;`)
* [Null pointer constant `nullptr`][cppr:cpp/language/nullptr]
(`int *i = nullptr;`)
* [Strongly-typed enums][scppr:enum class]
(`enum class E { ... };`)
* [Forward declaration for enums][scppr:enum fwd]
(`enum E;
...;
enum E { ... };`)
* [Generalized constant expressions][cppr:cpp/language/constexpr]
(`constexpr int a = 0;`)
* [Explicit conversion operators][cppr:cpp/language/cast_operator]
(`explicit operator bool();`)
* [Defaulted and deleted functions][cppr:cpp/language/function#Function_definition]
(`void a(int b) = delete;`)
* [Unique pointer template std::unique\_ptr<T\>][cppr:cpp/memory/unique_ptr]
(`std::unique_ptr<int> i;
std::unique_ptr<int[]> j;`)
* [Static assertions][cppr:cpp/language/static_assert]
* `std::addressof`

# Required C++14 features
* [`std::index_sequence`][cppr:cpp/utility/integer_sequence] is a compile-time sequence of integers.
* [`std::exchange`][cppr:cpp/utility/exchange] is a utility to update a variable, and yield the value it had before the update
* [`std::make_unique`][cppr:cpp/memory/unique_ptr/make_unique] is a convenience utility function for constructing `std::unique_ptr` with a managed value.

# Optional C++11/C++14 features
DXX-Rebirth code may use C++11 or C++14 features not present in the minimum supported compiler if the feature can be emulated easily (C++11: [inheriting constructors][cppr:cpp/language/using_declaration], [Range-based for][cppr:cpp/language/range-for]) or if the feature can be removed by a macro and the removal does not change the correctness of the program (C++11: [rvalue-qualified member methods][scppr:rvalue method]).

## Optional C++11 features
### Emulated if absent

* [Inheriting constructors][cppr:cpp/language/using_declaration] are wrapped by the macro `DXX_INHERIT_CONSTRUCTORS`.
If inherited constructors are supported, then `DXX_INHERIT_CONSTRUCTORS` expands to a `using` declaration.
If inherited constructors are not supported, then `DXX_INHERIT_CONSTRUCTORS` defines a [variadic template][cppr:cpp/language/parameter_pack] constructor to forward arguments to the base class.
* [Range-based for][cppr:cpp/language/range-for] is wrapped by the macro `range_for`.
This feature is present in the current minimum supported compiler versions.
It was first used when >=gcc-4.5 was the minimum.
Use of the `range_for` macro continues because it improves readability.

### Preprocessed out if absent

* [Reference-qualified methods][scppr:rvalue method] check that an rvalue which may or may not hold a valid pointer is not used in a context where the caller assumes the rvalue holds a valid pointer.
When the rvalue may or may not hold a valid pointer, it must be saved to an lvalue, tested for a valid pointer, and used only if a valid pointer is found.

[cppr:cpp/language/reference]: https://en.cppreference.com/w/cpp/language/reference
[cppr:cpp/language/parameter_pack]: https://en.cppreference.com/w/cpp/language/parameter_pack
[cppr:cpp/language/auto]: https://en.cppreference.com/w/cpp/language/auto
[cppr:cpp/language/lambda]: https://en.cppreference.com/w/cpp/language/lambda
[cppr:cpp/language/decltype]: https://en.cppreference.com/w/cpp/language/decltype
[cppr:cpp/language/type_alias]: https://en.cppreference.com/w/cpp/language/type_alias
[cppr:cpp/language/nullptr]: https://en.cppreference.com/w/cpp/language/nullptr
[scppr:enum class]: https://en.cppreference.com/w/cpp/language/enum#Scoped_enumerations.28since_C.2B.2B11.29
[scppr:enum fwd]: https://en.cppreference.com/w/cpp/language/enum#Unscoped_enumeration
[cppr:cpp/language/constexpr]: https://en.cppreference.com/w/cpp/language/constexpr
[cppr:cpp/language/cast_operator]: https://en.cppreference.com/w/cpp/language/cast_operator
[cppr:cpp/language/function#Function_definition]: https://en.cppreference.com/w/cpp/language/function#Function_definition
[cppr:cpp/memory/unique_ptr]: https://en.cppreference.com/w/cpp/memory/unique_ptr
[cppr:cpp/utility/exchange]: https://en.cppreference.com/w/cpp/utility/exchange
[cppr:cpp/utility/integer_sequence]: https://en.cppreference.com/w/cpp/utility/integer_sequence
[cppr:cpp/memory/unique_ptr/make_unique]: https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
[scppr:rvalue method]: https://en.cppreference.com/w/cpp/language/member_functions#const-.2C_volatile-.2C_and_ref-qualified_member_functions
[cppr:cpp/language/using_declaration]: https://en.cppreference.com/w/cpp/language/using_declaration
[cppr:cpp/language/range-for]: https://en.cppreference.com/w/cpp/language/range-for
[cppr:cpp/language/static_assert]: https://en.cppreference.com/w/cpp/language/static_assert
