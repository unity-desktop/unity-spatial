/* preview.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>

#include <wayfire/signal-definitions.hpp>
#include <wayfire/util.hpp>
#include <wayfire/unstable/wlr-subsurface-controller.hpp>

#include "preview-node.hpp"

namespace unity_spatial_preview
{
class preview
{
  public:
    preview(wlr_surface *surface, wayfire_view view, std::function<void()> closed);
    ~preview();

    preview(const preview&) = delete;
    preview& operator =(const preview&) = delete;

    void set_alpha(float alpha);

  private:
    void commit();
    void attach();
    void detach();
    void close();

    wlr_surface *surface;
    std::function<void()> closed;
    std::shared_ptr<preview_node_t> node;
    std::weak_ptr<wf::wlr_subsurface_root_node_t> root;
    float pending_alpha = 1.0f;

    wf::wl_listener_wrapper on_commit;
    wf::wl_listener_wrapper on_surface_destroy;
    wf::wl_listener_wrapper on_subsurface_destroy;
    wf::signal::connection_t<wf::view_unmapped_signal> on_unmapped;
};
}
