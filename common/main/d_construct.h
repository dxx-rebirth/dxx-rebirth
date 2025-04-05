/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <functional>	// std::invoke
#include <memory>	// std::addressof
#include <new>	// placement new
#include <type_traits>	// std::same_as
#include <utility>	// std::forward

/* Given a live variable `result` of type `result_type`, destroy it, and
 * construct a new `result_type` into that location from the result of calling
 * `std::invoke(invocable, args...)`.  This is similar to `std::construct_at`,
 * but:
 * - `std::construct_at` requires the storage not to be live, since it does not
 *   invoke the destructor before constructing the new object.
 * - `std::construct_at` requires its caller to provide the arguments passed to
 *   the `result_type` constructor.  In the specific case that the caller wants
 *   to use an `invocable` that returns a `result_type` by value, the
 *   `std::construct_at` will be called with a `result_type` that is then
 *   used to construct the `result`.  This extra layer defeats copy elision.
 *   By performing the invocation from inside `reconstruct_at`, the compiler is
 *   given the ability to apply copy elision.
 */
template <typename result_type, typename Invocable, typename... Args>
requires(
	requires(Invocable &&invocable, Args &&... args) {
		/* Require that `args` can be used to invoke `invocable`.  Failure to
		 * satisfy this would lead to a compile error later.  Checking it here
		 * is not required for correctness, but produces better reporting of
		 * errors, and is a side effect of the main test.
		 *
		 * Require that the type of the result of that invocation is exactly
		 * `result_type`.  This is required for optimal results, since an
		 * `invocable` that returns a type `T` that satisfies
		 * `(!std::same_as<T, result_type> && std::convertible_to<T, result_type>)`
		 * could satisfy the placement new in the body by converting the `T` to
		 * `result_type`, but would generate undesirable code.  Consider:

```
struct A { int a[100]; };
struct B : A { int b; };
B invocable();
void use_reconstruct(A &a)
{
	reconstruct_at(a, invocable);
}
```

		 * The call to `invocable` will return a `B`, which cannot be
		 * constructed directly into `a`, so the compiler will construct an
		 * anonymous temporary `B`, then construct `a` from that temporary `B`.
		 * In the shown example, that would require local storage for a `B`,
		 * and would copy the returned `int[100]` from local storage to the
		 * caller's `a`.  Avoid that overhead by rejecting parameters that
		 * would require the temporary.
		 */
		{ std::invoke(std::move(invocable), std::forward<Args>(args)...) } -> std::same_as<result_type>;
		/* Require that the object to be replaced has a trivial destructor or
		 * that the construction of the replacement object be `noexcept`.  A
		 * non-trivial destructor might malfunction if the object is destroyed
		 * twice.  Normally, there should be only one call, but if
		 * `result.~result_type()` finishes, then an exception is thrown during
		 * the call to `invocable`, the caller of this function could try to
		 * destroy `result` again, while it is still in the state that the
		 * destructor left it.  Therefore, require that if `result_type` has a
		 * non-trivial destructor, then construction cannot throw, so that
		 * there cannot be an exception during the period when `result` is in
		 * an inconsistent state.
		 *
		 * This check is overly cautious, since a non-trivial destructor might
		 * be safe to call twice.  There is no generic way to detect that and,
		 * for now, there are no types that fail to satisfy this check and for
		 * which `reconstruct_at` would be useful, so use the overly cautious
		 * check.
		 */
		requires(std::is_trivially_destructible_v<result_type> || noexcept(result_type{std::invoke(std::move(invocable), std::forward<Args>(args)...)}));
	}
)
static inline void reconstruct_at(result_type &result, Invocable &&invocable, Args &&... args)
{
	/* Destroy the old object, since its storage will be reused. */
	result.~result_type();
	/* Construct a new `result_type` in the location `result`, using a
	 * constructor that takes the result of `std::invoke(...)`.  Since the
	 * above `requires` clause requires that `std::invoke` return a
	 * `result_type`, this should result in copy elision constructing the new
	 * `result_type` directly into `result`, bypassing any copy constructor or
	 * move constructor.
	 */
	new(static_cast<void *>(std::addressof(result))) result_type{std::invoke(std::move(invocable), std::forward<Args>(args)...)};
}
