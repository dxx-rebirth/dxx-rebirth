#include "dxxsconf.h"
#include "compiler-poison.h"

/* If DXX_HAVE_POISON_UNDEFINED is not zero,
 * array_managed_type::array_managed_type() is declared and must be
 * defined out of line.  These tests do not need to run any code at
 * array construction time, so force DXX_HAVE_POISON_UNDEFINED to be zero.
 */
#undef DXX_HAVE_POISON_UNDEFINED
#define DXX_HAVE_POISON_UNDEFINED	0
#include "valptridx.h"
#include "valptridx.tcc"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth valptridx
#include <boost/test/unit_test.hpp>

/* Convenience macro to prevent the compiler optimizer from performing
 * constant propagation of the input.  This ensures that build-time
 * tests are suppressed, allowing the runtime test to be emitted and
 * executed.
 */
#define OPTIMIZER_HIDE_VARIABLE(V)	asm("" : "=rm" (V) : "0" (V) : "memory")

/* Convenience function to pass an anonymous value (such as an integer
 * constant) through OPTIMIZER_HIDE_VARIABLE, then return it.  This is
 * needed for contexts where a bare asm() statement would not be legal.
 *
 * It also has the useful secondary effect of casting the input to a
 * type, if the caller explicitly sets the typename T.
 */
template <typename T>
static inline T optimizer_hidden_variable(T v)
{
	OPTIMIZER_HIDE_VARIABLE(v);
	return v;
}

/* Convenience macro to ignore the result of an expression in a way that
 * __attribute__((warn_unused_result)) does not warn that the result was
 * ignored.
 */
#define DXX_TEST_VALPTRIDX_IGNORE_RETURN(EXPR)	({	auto &&r = EXPR; static_cast<void>(r); })

struct object
{
	/* No state needed - this module is testing valptridx, not the type
	 * managed by valptridx.
	 */
};

using objnum_t = uint16_t;
constexpr std::integral_constant<std::size_t, 350> MAX_OBJECTS{};
DXX_VALPTRIDX_DECLARE_SUBTYPE(, object, objnum_t, MAX_OBJECTS);
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(object, obj);
DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(object, obj, Objects);

valptridx<object>::array_managed_type Objects;

/* Some valptridx names are normally protected.  The tests need access
 * to them.  Define a subclass that relaxes the permissions.
 */
struct valptridx_access_override : valptridx<object>
{
	using base_type = valptridx<object>;
	using typename base_type::allow_end_construction;
	using typename base_type::index_range_exception;
	using typename base_type::null_pointer_exception;
	using typename base_type::report_error_uses_exception;
};

/* Test that using the factory does not throw an exception on the
 * highest valid index.
 */
BOOST_AUTO_TEST_CASE(idx_no_exception_near_end)
{
	BOOST_CHECK_NO_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(
			Objects.vmptr(optimizer_hidden_variable<objnum_t>(MAX_OBJECTS - 1)))
		);
}

/* Test that using the factory does throw an exception on the lowest
 * invalid index and on the highest representable integer.
 */
BOOST_AUTO_TEST_CASE(idx_exception_at_end)
{
	if (!valptridx_access_override::report_error_uses_exception::value)
		/* If valptridx is configured not to use exceptions to report
		 * errors, these tests will fail even when the logic is correct.
		 * Skip them in that case.
		 */
		return;
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(
			Objects.vmptr(
				optimizer_hidden_variable<objnum_t>(MAX_OBJECTS))),
		valptridx_access_override::index_range_exception);

	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(
			Objects.vmptr(
				optimizer_hidden_variable<objnum_t>(UINT16_MAX))),
		valptridx_access_override::index_range_exception);
}

BOOST_AUTO_TEST_CASE(iter_no_exception_at_allowed_end)
{
	/* Test the allowed side of end() handling for off-by-one bugs.
	 */
	BOOST_CHECK_NO_THROW(DXX_TEST_VALPTRIDX_IGNORE_RETURN(
			(Objects.set_count(optimizer_hidden_variable<objnum_t>(MAX_OBJECTS - 1)), Objects.vmptr.end())
	));

	BOOST_CHECK_NO_THROW(DXX_TEST_VALPTRIDX_IGNORE_RETURN(
			(Objects.set_count(optimizer_hidden_variable<objnum_t>(MAX_OBJECTS)), Objects.vmptr.end())
	));
}

BOOST_AUTO_TEST_CASE(iter_exception_if_beyond_end)
{
	/* Test that end() throws an exception if the range's count exceeds
	 * the valid storage size.
	 */
	if (!valptridx_access_override::report_error_uses_exception::value)
		return;
	BOOST_CHECK_THROW((DXX_TEST_VALPTRIDX_IGNORE_RETURN(
				(Objects.set_count(optimizer_hidden_variable<objnum_t>(MAX_OBJECTS + 1)), Objects.vmptr.end())
	)), valptridx_access_override::index_range_exception);
}

BOOST_AUTO_TEST_CASE(guarded_in_range)
{
	auto &&g = Objects.vmptr.check_untrusted(optimizer_hidden_variable<objnum_t>(0));
	optimizer_hidden_variable(g);
	/* Test that a `guarded` returned by `check_untrusted` on a valid
	 * index throws if accessed without testing its validity.  A runtime
	 * check on validity is mandatory.
	 *
	 * These exceptions are not configurable, so no test on
	 * `report_error_uses_exception` is required.
	 */
	BOOST_CHECK_THROW(DXX_TEST_VALPTRIDX_IGNORE_RETURN(*g), std::logic_error);
	/* Test its validity */
	BOOST_TEST(static_cast<bool>(g));
	/* Test that it can now be read successfully. */
	BOOST_CHECK_NO_THROW(DXX_TEST_VALPTRIDX_IGNORE_RETURN(*g));
}

BOOST_AUTO_TEST_CASE(guarded_out_of_range)
{
	auto &&g = Objects.vmptr.check_untrusted(optimizer_hidden_variable<objnum_t>(MAX_OBJECTS));
	optimizer_hidden_variable(g);
	/* Test that a `guarded` returned by `check_untrusted` on an invalid
	 * index throws if accessed without testing its validity.
	 *
	 * Test its validity.
	 *
	 * Test that it still throws afterward.
	 */
	BOOST_CHECK_THROW(DXX_TEST_VALPTRIDX_IGNORE_RETURN(*g), std::logic_error);
	BOOST_TEST(!static_cast<bool>(g));
	BOOST_CHECK_THROW(DXX_TEST_VALPTRIDX_IGNORE_RETURN(*g), std::logic_error);
}

BOOST_AUTO_TEST_CASE(idx_convert_check)
{
	using vo = valptridx<object>;
	BOOST_CHECK_NO_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			typename vo::imidx i(static_cast<objnum_t>(0));
			typename vo::vmidx{i};
			}))
	);
	typename vo::imidx i_none(static_cast<objnum_t>(~0));
	if (!valptridx_access_override::report_error_uses_exception::value)
		return;
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			typename vo::vmidx{i_none};
			})), valptridx_access_override::index_range_exception
	);
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			typename vo::vmidx{optimizer_hidden_variable<objnum_t>(~0)};
			})), valptridx_access_override::index_range_exception
	);
}

BOOST_AUTO_TEST_CASE(ptr_convert_check)
{
	using vo = valptridx<object>;
	typename vo::imptr i_none(nullptr);
	auto &&i_zero = Objects.imptr(optimizer_hidden_variable<objnum_t>(0));
	BOOST_CHECK_NO_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			typename vo::vmptr v_zero{i_zero};
			v_zero.get_nonnull_pointer();
			}))
	);
	if (!valptridx_access_override::report_error_uses_exception::value)
		return;
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			typename vo::vmptr{i_none};
			})), valptridx_access_override::null_pointer_exception
	);
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			Objects.vmptr(optimizer_hidden_variable<object *>(nullptr));
			})), valptridx_access_override::null_pointer_exception
	);
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			i_none.get_nonnull_pointer();
			})), valptridx_access_override::null_pointer_exception
	);
	auto &&pi_zero = Objects.vmptridx(optimizer_hidden_variable<objnum_t>(0));
	BOOST_CHECK_THROW(
		DXX_TEST_VALPTRIDX_IGNORE_RETURN(({
			pi_zero.absolute_sibling(optimizer_hidden_variable<objnum_t>(Objects.size()));
			})), valptridx_access_override::index_range_exception
	);
}
