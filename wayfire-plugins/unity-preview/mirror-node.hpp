/* mirror-node.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <wayfire/geometry.hpp>
#include <wayfire/view.hpp>
#include <wayfire/view-transform.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>

namespace unity_preview
{
class mirror_node_t : public wf::scene::transformer_base_node_t
{
  public:
    mirror_node_t(wayfire_view source, wlr_surface *host);

    wf::geometry_t get_host_bounds() const;

    wf::geometry_t get_bounding_box() override;
    void gen_render_instances(std::vector<wf::scene::render_instance_uptr>& instances,
        wf::scene::damage_callback push_damage, wf::output_t *shown_on) override;
    std::string stringify() const override;

    std::weak_ptr<wf::view_interface_t> source;
    wf::geometry_t rect{0, 0, 0, 0};
    float alpha   = 1.0f;
    int32_t order = 0;

  private:
    wlr_surface *host;
};
}
