/* preview-node.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <wayfire/view.hpp>
#include <wayfire/view-transform.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>

namespace unity_spatial_preview
{
class preview_node_t : public wf::scene::transformer_base_node_t
{
  public:
    preview_node_t(wayfire_view source, wlr_surface *surface);

    wf::geometry_t get_bounding_box() override;
    wf::geometry_t get_clip() const;
    void gen_render_instances(std::vector<wf::scene::render_instance_uptr>& instances,
        wf::scene::damage_callback push_damage, wf::output_t *shown_on) override;
    std::string stringify() const override;

    const std::weak_ptr<wf::view_interface_t> source;
    wlr_surface *const surface;
    float alpha = 1.0f;

  private:
    bool generating = false;
};
}
