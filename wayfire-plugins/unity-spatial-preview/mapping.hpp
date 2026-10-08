/* mapping.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <optional>

#include <wayfire/geometry.hpp>

namespace unity_spatial_preview
{
struct mapping_t
{
    wf::geometry_t box;
    wf::geometry_t source;
};

std::optional<mapping_t> map_window(const wf::geometry_t& frame, const wf::geometry_t& bounds, double width,
    double height);
}
