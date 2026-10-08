/* mapping.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mapping.hpp"

#include <algorithm>
#include <cmath>

namespace unity_spatial_preview
{
std::optional<mapping_t> map_window(const wf::geometry_t& frame, const wf::geometry_t& bounds, double width,
    double height)
{
    if ((frame.width <= 0) || (frame.height <= 0) || (width <= 0) || (height <= 0))
    {
        return std::nullopt;
    }

    double sx     = width / frame.width;
    double sy     = height / frame.height;
    double left   = std::max(0.0, std::floor((frame.x - bounds.x) * sx));
    double top    = std::max(0.0, std::floor((frame.y - bounds.y) * sy));
    double right  = std::max(0.0, std::floor((bounds.x + bounds.width - frame.x - frame.width) * sx));
    double bottom = std::max(0.0, std::floor((bounds.y + bounds.height - frame.y - frame.height) * sy));

    mapping_t mapping;
    mapping.box    = {-left, -top, width + left + right, height + top + bottom};
    mapping.source = {frame.x - bounds.x - left / sx, frame.y - bounds.y - top / sy, mapping.box.width / sx,
        mapping.box.height / sy};

    return mapping;
}
}
