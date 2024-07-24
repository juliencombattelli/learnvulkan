#pragma once

#include <algorithm>
#include <utility>

namespace stdx::ranges {

template<typename TContainer, typename TValue, typename TProjector = std::identity>
[[nodiscard]] bool contains(TContainer&& container, TValue&& value, TProjector projector = {})
{
    return std::ranges::find(
               std::forward<TContainer>(container),
               std::forward<TValue>(value),
               projector)
        != container.end();
}

template<std::ranges::random_access_range TRange>
void sort_unique(TRange& range)
{
    std::sort(range.begin(), range.end());
    range.erase(std::unique(range.begin(), range.end()), range.end());
}

} // namespace stdx::ranges
