/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 */

#pragma once

#include "maths.h"
#include "dxxsconf.h"
#include "dsx-ns.h"

namespace dcx {

struct vms_vector;

class vm_distance;
class vm_magnitude;
class vm_distance_squared;
enum class vm_magnitude_squared : uint64_t;
struct vms_angvec;
struct vms_matrix;
struct vms_quaternion;

extern const vms_matrix vmd_identity_matrix;

#define IDENTITY_MATRIX vms_matrix{	\
	.rvec = {F1_0, 0, 0},	\
	.uvec = {0, F1_0, 0},	\
	.fvec = {0, 0, F1_0}	\
}

vms_vector &vm_vec_add (vms_vector &dest, const vms_vector &src0, const vms_vector &src1);

vms_vector &_vm_vec_sub(vms_vector &dest, const vms_vector &src0, const vms_vector &src1);
void vm_vec_add2 (vms_vector &dest, const vms_vector &src);
void vm_vec_sub2 (vms_vector &dest, const vms_vector &src);
vms_vector &vm_vec_scale (vms_vector &dest, fix s);

#define vm_vec_copy_scale(A,B,...)	vm_vec_copy_scale(A, ## __VA_ARGS__, B)
vms_vector &vm_vec_copy_scale (vms_vector &dest, const vms_vector &src, fix s);
void vm_vec_scale_add (vms_vector &dest, const vms_vector &src1, const vms_vector &src2, fix k);
void vm_vec_scale_add2 (vms_vector &dest, const vms_vector &src, fix k);
void vm_vec_scale2 (vms_vector &dest, fix n, fix d);

[[nodiscard]]
vm_magnitude_squared vm_vec_mag2(const vms_vector &v);

[[nodiscard]]
vm_magnitude vm_vec_mag(const vms_vector &v);

[[nodiscard]]
vm_distance vm_vec_dist(const vms_vector &v0, const vms_vector &v1);

[[nodiscard]]
vm_distance_squared vm_vec_dist2(const vms_vector &v0, const vms_vector &v1);

[[nodiscard]]
vm_magnitude vm_vec_mag_quick(const vms_vector &v);

[[nodiscard]]
vm_distance vm_vec_dist_quick(const vms_vector &v0, const vms_vector &v1);

[[nodiscard]]
vm_magnitude vm_vec_copy_normalize(vms_vector &dest, const vms_vector &src);

vm_magnitude vm_vec_normalize(vms_vector &v);

vm_magnitude vm_vec_copy_normalize_quick(vms_vector &dest, const vms_vector &src);

vm_magnitude vm_vec_normalize_quick(vms_vector &v);
vm_magnitude vm_vec_normalized_dir (vms_vector &dest, const vms_vector &end, const vms_vector &start);
vm_magnitude vm_vec_normalized_dir_quick (vms_vector &dest, const vms_vector &end, const vms_vector &start);

[[nodiscard]]
fix vm_vec_dot (const vms_vector &v0, const vms_vector &v1);

[[nodiscard]]
fixang vm_vec_delta_ang (const vms_vector &v0, const vms_vector &v1, const vms_vector &fvec);

[[nodiscard]]
fixang vm_vec_delta_ang_norm (const vms_vector &v0, const vms_vector &v1, const vms_vector &fvec);

void vm_angles_2_matrix (vms_matrix &m, const vms_angvec &a);

#if DXX_USE_EDITOR
void vm_vec_ang_2_matrix (vms_matrix &m, const vms_vector &v, fixang a);
#endif

void vm_vector_2_matrix (vms_matrix &m, const vms_vector &fvec, const vms_vector *uvec, const vms_vector *rvec);
void vm_vec_rotate (vms_vector &dest, const vms_vector &src, const vms_matrix &m);
void _vm_matrix_x_matrix (vms_matrix &dest, const vms_matrix &src0, const vms_matrix &src1);
void vm_extract_angles_matrix (vms_angvec &a, const vms_matrix &m);
void vm_extract_angles_vector (vms_angvec &a, const vms_vector &v);

[[nodiscard]]
fix vm_dist_to_plane (const vms_vector &checkp, const vms_vector &norm, const vms_vector &planep);

void vms_quaternion_from_matrix(vms_quaternion &q, const vms_matrix &m);
void vms_matrix_from_quaternion(vms_matrix &m, const vms_quaternion &q);

}
