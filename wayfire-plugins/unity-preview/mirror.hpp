/* mirror.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <memory>

#include <wayfire/geometry.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/util.hpp>
#include <wayfire/view.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>

namespace unity_preview
{
struct preview_state
{
    wf::geometry_t rect{0, 0, 0, 0};
    float alpha   = 1.0f;
    int32_t order = 0;
};

class mirror
{
  public:
    mirror(wl_resource *resource, wayfire_view source, wl_resource *surface);
    ~mirror();

    mirror(const mirror&) = delete;
    mirror& operator =(const mirror&) = delete;

    void set_rect(const wf::geometry_t& rect);
    void set_alpha(float alpha);
    void set_order(int32_t order);

  private:
    class attachment;

    struct synced_link
    {
        wlr_surface_synced synced;
        mirror *owner;
    };

    static void init_state(void *state);
    static void commit_state(wlr_surface_synced *synced);
    static const wlr_surface_synced_impl synced_impl;

    void attach(wayfire_view host_view);
    void detach();
    void close();
    void release_host();
    void send_source_size();

    wl_resource *resource;
    wlr_surface *host = nullptr;
    std::weak_ptr<wf::view_interface_t> source;
    synced_link link{};
    preview_state pending;
    preview_state current;
    wf::dimensions_t source_size{0, 0};
    std::unique_ptr<attachment> attached;

    wf::wl_listener_wrapper on_host_destroy;
    wf::signal::connection_t<wf::view_mapped_signal> on_view_mapped;
    wf::signal::connection_t<wf::view_unmapped_signal> on_host_unmapped;
    wf::signal::connection_t<wf::view_unmapped_signal> on_source_unmapped;
    wf::signal::connection_t<wf::view_geometry_changed_signal> on_source_geometry_changed;
};
}
