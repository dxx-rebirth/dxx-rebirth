/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "object.h"
#include <variant>

namespace dcx {
enum class gauge_inset_window_view : unsigned;
}

#ifdef DXX_BUILD_DESCENT
namespace dsx {

// Keep demo inset identity stable across stereo eyes without naming a world object.
using render_view_identity = std::variant<const object *, ::dcx::gauge_inset_window_view>;

// A rendering camera is a value; only live views name a managed source object.
struct render_view
{
	object_type type;
	segnum_t segnum;
	vms_vector pos;
	vms_matrix orient;
	render_view_identity identity;

	explicit render_view(const object &source) :
		type(source.type), segnum(source.segnum), pos(source.pos), orient(source.orient), identity(&source)
	{
	}

	render_view(const object_type type, const segnum_t segnum, const vms_vector &pos, const vms_matrix &orient, const ::dcx::gauge_inset_window_view window) :
		type(type), segnum(segnum), pos(pos), orient(orient), identity(window)
	{
	}

	const object *source_object() const
	{
		const auto source = std::get_if<const object *>(&identity);
		return source ? *source : nullptr;
	}
};

}
#endif
