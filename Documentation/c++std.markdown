DXX-Rebirth requires a compiler that implements the C++20 standard.  A fully conforming compiler is recommended, but some omissions can be handled by SConf tests that enable a fallback to emulate the feature.

# Required C++11 features
DXX-Rebirth code uses C++20 features present in >=clang-15.0 and >=gcc-12.  Some of these features are probed in the SConf tests so that an error can be reported if the feature is missing.  However, since these are considered the minimum supported compiler versions, and existing SConf tests reject older compilers, some C++20 features that are new in gcc-12 may be used without a corresponding test in SConf.

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
* [inheriting constructors][cppr:cpp/language/using_declaration]
* [Range-based for][cppr:cpp/language/range-for]
* [Reference-qualified methods][scppr:rvalue method] check that an rvalue which may or may not hold a valid pointer is not used in a context where the caller assumes the rvalue holds a valid pointer.

# Required C++14 features
* [`std::index_sequence`][cppr:cpp/utility/integer_sequence] is a compile-time sequence of integers.
* [`std::exchange`][cppr:cpp/utility/exchange] is a utility to update a variable, and yield the value it had before the update
* [`std::make_unique`][cppr:cpp/memory/unique_ptr/make_unique] is a convenience utility function for constructing `std::unique_ptr` with a managed value.

# Required C++17 features
* [Fold expressions][cppr:cpp/language/fold]
* [Class template argument deduction][cppr:cpp/language/class_template_argument_deduction]
* [`if constexpr`][cppr:cpp/language/if]
* [inline variables][cppr:cpp/language/inline]
* [structured bindings][cppr:cpp/language/structured_binding]
* [initializers in `if`][cppr:cpp/language/if]
* [attribute `[[fallthrough]]`][cppr:cpp/language/attributes/fallthrough]
* [attribute `[[maybe_unused]]`][cppr:cpp/language/attributes/maybe_unused]
* [attribute `[[nodiscard]]`][cppr:cpp/language/attributes/nodiscard]
* [`std::optional`][cppr:cpp/utility/optional]
* [non-member `std::size()`][cppr:cpp/iterator/size]
* [non-member `std::data()`][cppr:cpp/iterator/data]

# Required C++20 features
* [3-way comparison `operator<=>`][scppr:operator_comparison]
* [explicitly defaulted `operator==()`][cppr:cpp/language/default_comparisons]
* [designated initializers][cppr:cpp/language/aggregate_initialization#Designated_initializers]
* [constraints and concepts][cppr:cpp/language/constraints]
* [`std::span`][cppr:cpp/container/span]
* [`std::endian`][cppr:cpp/types/endian]

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
[cppr:cpp/language/fold]: https://en.cppreference.com/w/cpp/language/fold
[cppr:cpp/language/class_template_argument_deduction]: https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
[cppr:cpp/language/if]: https://en.cppreference.com/w/cpp/language/if
[cppr:cpp/language/inline]: https://en.cppreference.com/w/cpp/language/inline
[cppr:cpp/language/structured_binding]: https://en.cppreference.com/w/cpp/language/structured_binding
[cppr:cpp/language/if]: https://en.cppreference.com/w/cpp/language/if
[cppr:cpp/language/attributes/fallthrough]: https://en.cppreference.com/w/cpp/language/attributes/fallthrough
[cppr:cpp/language/attributes/maybe_unused]: https://en.cppreference.com/w/cpp/language/attributes/maybe_unused
[cppr:cpp/language/attributes/nodiscard]: https://en.cppreference.com/w/cpp/language/attributes/nodiscard
[cppr:cpp/utility/optional]: https://en.cppreference.com/w/cpp/utility/optional
[cppr:cpp/iterator/size]: https://en.cppreference.com/w/cpp/iterator/size
[cppr:cpp/iterator/data]: https://en.cppreference.com/w/cpp/iterator/data
[scppr:operator_comparison]: https://en.cppreference.com/w/cpp/language/operator_comparison#Three-way_comparison
[cppr:cpp/language/default_comparisons]: https://en.cppreference.com/w/cpp/language/default_comparisons
[cppr:cpp/language/aggregate_initialization#Designated_initializers]: https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers
[cppr:cpp/language/constraints]: https://en.cppreference.com/w/cpp/language/constraints
[cppr:cpp/container/span]: https://en.cppreference.com/w/cpp/container/span
[cppr:cpp/types/endian]: https://en.cppreference.com/w/cpp/types/endian
