/* preview.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <wayfire/signal-definitions.hpp>
#include <wayfire/util.hpp>

#include "preview-node.hpp"

namespace unity_spatial_preview
{
class preview
{
  public:
    preview(wlr_subsurface *subsurface, wayfire_view view);
    ~preview();

    preview(const preview&) = delete;
    preview& operator =(const preview&) = delete;

  private:
    void attach();
    void sync();
    void close();

    wlr_subsurface *subsurface;
    std::shared_ptr<preview_node_t> node;

    wf::wl_listener_wrapper on_parent_commit;
    wf::wl_listener_wrapper on_commit;
    wf::wl_listener_wrapper on_map;
    wf::wl_listener_wrapper on_unmap;
    wf::wl_listener_wrapper on_subsurface_destroy;
    wf::signal::connection_t<wf::view_unmapped_signal> on_unmapped;
};
}
