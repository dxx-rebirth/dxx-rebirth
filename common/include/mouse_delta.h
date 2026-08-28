/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace dcx {

class mouse_delta_state
{
	std::array<int64_t, 3> m_delta{};
	bool m_pending{};
public:
	void reset()
	{
		m_delta = {};
		m_pending = false;
	}

	void update(const std::array<int, 3> &delta)
	{
		for (std::size_t i = 0; i != m_delta.size(); ++i)
			m_delta[i] = delta[i];
		m_pending = true;
	}

	[[nodiscard]] std::optional<std::array<int64_t, 3>> consume()
	{
		if (!m_pending)
			return std::nullopt;
		m_pending = false;
		return std::exchange(m_delta, {});
	}
};

}
