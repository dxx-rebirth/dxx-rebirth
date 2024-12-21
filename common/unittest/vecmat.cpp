#include "vecmat.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth vecmat
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(vm_vec_add_0)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	const auto a2{a};
	vms_vector d{};
	auto &r{vm_vec_add(d, a, vms_vector{.x = 15, .y = 100, .z = 250})};
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(&r, &d);
	BOOST_CHECK_EQUAL(d.x, 25);
	BOOST_CHECK_EQUAL(d.y, 120);
	BOOST_CHECK_EQUAL(d.z, 280);
}

BOOST_AUTO_TEST_CASE(vm_vec_sub_0)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	const auto a2{a};
	vms_vector d{};
	auto &r{vm_vec_sub(d, a, vms_vector{.x = 15, .y = 5, .z = 250})};
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(&r, &d);
	BOOST_CHECK_EQUAL(d.x, -5);
	BOOST_CHECK_EQUAL(d.y, 15);
	BOOST_CHECK_EQUAL(d.z, -220);
}

BOOST_AUTO_TEST_CASE(vm_vec_add2_mixed)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	const auto a2{a};
	vms_vector d{.x = 15, .y = 100, .z = 250};
	vm_vec_add2(d, a);
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(d.x, 25);
	BOOST_CHECK_EQUAL(d.y, 120);
	BOOST_CHECK_EQUAL(d.z, 280);
}

BOOST_AUTO_TEST_CASE(vm_vec_add2_self)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	const auto a2{a};
	vm_vec_add2(a, a);
	BOOST_CHECK_EQUAL(a.x, a2.x * 2);
	BOOST_CHECK_EQUAL(a.y, a2.y * 2);
	BOOST_CHECK_EQUAL(a.z, a2.z * 2);
}

BOOST_AUTO_TEST_CASE(vm_vec_sub2_mixed)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	const auto a2{a};
	vms_vector d{.x = 15, .y = 100, .z = 250};
	vm_vec_sub2(d, a);
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(d.x, 5);
	BOOST_CHECK_EQUAL(d.y, 80);
	BOOST_CHECK_EQUAL(d.z, 220);
}

BOOST_AUTO_TEST_CASE(vm_vec_sub2_self)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	vm_vec_sub2(a, a);
	BOOST_CHECK_EQUAL(a.x, 0);
	BOOST_CHECK_EQUAL(a.y, 0);
	BOOST_CHECK_EQUAL(a.z, 0);
}

BOOST_AUTO_TEST_CASE(vm_vec_avg_positive)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	const auto a2{a};
	vms_vector b{.x = 100, .y = 205, .z = 300};
	const auto b2{b};
	vms_vector d{vm_vec_avg(a, b)};
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(b.x, b2.x);
	BOOST_CHECK_EQUAL(b.y, b2.y);
	BOOST_CHECK_EQUAL(b.z, b2.z);
	BOOST_CHECK_EQUAL(d.x, 55);
	BOOST_CHECK_EQUAL(d.y, 112);	/* Integer division */
	BOOST_CHECK_EQUAL(d.z, 165);
}

BOOST_AUTO_TEST_CASE(vm_vec_avg_negative)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	vms_vector b{.x = -100, .y = -205, .z = -300};
	vms_vector d{vm_vec_avg(a, b)};
	BOOST_CHECK_EQUAL(d.x, -45);
	BOOST_CHECK_EQUAL(d.y, -92);	/* Integer division */
	BOOST_CHECK_EQUAL(d.z, -135);
}

BOOST_AUTO_TEST_CASE(vm_vec_scale_positive)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	auto &r{vm_vec_scale(a, 5 << 16)};
	BOOST_CHECK_EQUAL(&r, &a);
	BOOST_CHECK_EQUAL(a.x, 50);
	BOOST_CHECK_EQUAL(a.y, 100);
	BOOST_CHECK_EQUAL(a.z, 150);
}

BOOST_AUTO_TEST_CASE(vm_vec_scale_negative_l)
{
	vms_vector a{.x = -11, .y = -20, .z = -30};
	auto &r{vm_vec_scale(a, 5 << 16)};
	BOOST_CHECK_EQUAL(&r, &a);
	BOOST_CHECK_EQUAL(a.x, -55);
	BOOST_CHECK_EQUAL(a.y, -100);
	BOOST_CHECK_EQUAL(a.z, -150);
}

BOOST_AUTO_TEST_CASE(vm_vec_scale_negative_r)
{
	vms_vector a{.x = 12, .y = 20, .z = 30};
	auto &r{vm_vec_scale(a, -5 << 16)};
	BOOST_CHECK_EQUAL(&r, &a);
	BOOST_CHECK_EQUAL(a.x, -60);
	BOOST_CHECK_EQUAL(a.y, -100);
	BOOST_CHECK_EQUAL(a.z, -150);
}

BOOST_AUTO_TEST_CASE(vm_vec_scale_negative_lr)
{
	vms_vector a{.x = -13, .y = -20, .z = -30};
	auto &r{vm_vec_scale(a, -5 << 16)};
	BOOST_CHECK_EQUAL(&r, &a);
	BOOST_CHECK_EQUAL(a.x, 65);
	BOOST_CHECK_EQUAL(a.y, 100);
	BOOST_CHECK_EQUAL(a.z, 150);
}

BOOST_AUTO_TEST_CASE(vm_vec_copy_scale_0)
{
	/* Test that vm_vec_copy_scale does not modify the input argument */
	vms_vector a{.x = 50, .y = 100, .z = 150};
	auto r{vm_vec_copy_scale(a, 5 << 16)};
	BOOST_CHECK_EQUAL(a.x, 50);
	BOOST_CHECK_EQUAL(a.y, 100);
	BOOST_CHECK_EQUAL(a.z, 150);
	BOOST_CHECK_EQUAL(r.x, 250);
	BOOST_CHECK_EQUAL(r.y, 500);
	BOOST_CHECK_EQUAL(r.z, 750);
}

BOOST_AUTO_TEST_CASE(vm_vec_scale_add_0)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	vms_vector b{.x = 60, .y = 70, .z = 80};
	const auto a2{a};
	const auto b2{b};
	auto r{vm_vec_scale_add(a, b, 5 << 16)};
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(b.x, b2.x);
	BOOST_CHECK_EQUAL(b.y, b2.y);
	BOOST_CHECK_EQUAL(b.z, b2.z);
	BOOST_CHECK_EQUAL(r.x, a.x + (b.x * 5));
	BOOST_CHECK_EQUAL(r.y, a.y + (b.y * 5));
	BOOST_CHECK_EQUAL(r.z, a.z + (b.z * 5));
}

BOOST_AUTO_TEST_CASE(vm_vec_scale_add2_0)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	vms_vector b{.x = 60, .y = 70, .z = 80};
	const auto a2{a};
	const auto b2{b};
	auto r{a};
	vm_vec_scale_add2(r, b, 5 << 16);
	BOOST_CHECK_EQUAL(a.x, a2.x);
	BOOST_CHECK_EQUAL(a.y, a2.y);
	BOOST_CHECK_EQUAL(a.z, a2.z);
	BOOST_CHECK_EQUAL(b.x, b2.x);
	BOOST_CHECK_EQUAL(b.y, b2.y);
	BOOST_CHECK_EQUAL(b.z, b2.z);
	BOOST_CHECK_EQUAL(r.x, a.x + (b.x * 5));
	BOOST_CHECK_EQUAL(r.y, a.y + (b.y * 5));
	BOOST_CHECK_EQUAL(r.z, a.z + (b.z * 5));
}

BOOST_AUTO_TEST_CASE(vm_vec_scale2_0)
{
	vms_vector a{.x = 10, .y = 20, .z = 30};
	vm_vec_scale2(a, 5, 3);
	BOOST_CHECK_EQUAL(a.x, 50 / 3);
	BOOST_CHECK_EQUAL(a.y, 100 / 3);
	BOOST_CHECK_EQUAL(a.z, 50);
}

BOOST_AUTO_TEST_CASE(vm_vec_dot_0)
{
	const vms_vector a{.x = 10 << 16, .y = 20 << 16, .z = 30 << 16};
	const vms_vector b{.x = 30 << 16, .y = 70 << 16, .z = 300 << 16};
	BOOST_CHECK_EQUAL(vm_vec_dot(a, b), ((10 * 30) + (20 * 70) + (30 * 300)) << 16);
	const vms_vector c{.x = 30 << 16, .y = -70 << 16, .z = 300 << 16};
	BOOST_CHECK_EQUAL(vm_vec_dot(a, c), ((10 * 30) + (20 * -70) + (30 * 300)) << 16);
}

BOOST_AUTO_TEST_CASE(vm_vec_mag2_0)
{
	BOOST_CHECK_EQUAL(static_cast<int64_t>(vm_vec_mag2(vms_vector{.x = 10, .y = 20, .z = 30})), (10 * 10) + (20 * 20) + (30 * 30));
	BOOST_CHECK_EQUAL(static_cast<int64_t>(vm_vec_mag2(vms_vector{.x = 10, .y = -20, .z = 30})), (10 * 10) + (20 * 20) + (30 * 30));
}

BOOST_AUTO_TEST_CASE(vm_vec_mag_0)
{
	BOOST_CHECK_EQUAL(static_cast<int32_t>(vm_vec_mag(vms_vector{.x = 10, .y = 20, .z = 30})), 37);
	BOOST_CHECK_EQUAL(static_cast<int32_t>(vm_vec_mag(vms_vector{.x = 10, .y = -20, .z = 30})), 37);
	BOOST_CHECK_EQUAL(static_cast<int32_t>(vm_vec_mag(vms_vector{.x = 60, .y = 0, .z = 0})), 60);
	BOOST_CHECK_EQUAL(static_cast<int32_t>(vm_vec_mag(vms_vector{.x = 0, .y = 90, .z = 0})), 90);
	BOOST_CHECK_EQUAL(static_cast<int32_t>(vm_vec_mag(vms_vector{.x = 0, .y = 0, .z = 45})), 45);
}
