#pragma once

#include <concepts>
#include <utility>

namespace syssnap
{
	using cpu_t  = int;
	using node_t = int;

	static constexpr inline auto idx(const std::signed_integral auto i)
	{
		if (std::cmp_less(i, 0)) { throw std::out_of_range("Index must be greater than or equal to zero."); }
		return static_cast<std::size_t>(i); // NOLINT
	}
} // namespace syssnap