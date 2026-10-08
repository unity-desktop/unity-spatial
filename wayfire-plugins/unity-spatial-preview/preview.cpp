/* preview.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "preview.hpp"

#include <wayfire/scene-operations.hpp>
#include <wayfire/unstable/wlr-subsurface-controller.hpp>

namespace unity_spatial_preview
{
preview::preview(wlr_subsurface *subsurface, wayfire_view view) :
    subsurface(subsurface), node(std::make_shared<preview_node_t>(view, subsurface))
{
    on_subsurface_destroy.set_callback([this] (void*) { close(); });
    on_subsurface_destroy.connect(&subsurface->events.destroy);
    on_parent_commit.set_callback([this] (void*) { attach(); });
    on_parent_commit.connect(&subsurface->parent->events.commit);
    on_commit.set_callback([this] (void*) { sync(); });
    on_commit.connect(&subsurface->surface->events.commit);
    on_map.set_callback([this] (void*) { sync(); });
    on_map.connect(&subsurface->surface->events.map);
    on_unmap.set_callback([this] (void*) { sync(); });
    on_unmap.connect(&subsurface->surface->events.unmap);
    on_unmapped = [this] (auto) { close(); };
    view->connect(&on_unmapped);
    attach();
}

preview::~preview()
{
    close();
}

void preview::attach()
{
    if (!node || node->parent() || !subsurface->data)
    {
        return;
    }

    on_parent_commit.disconnect();
    auto *controller = static_cast<wf::wlr_subsurface_controller_t*>(subsurface->data);
    wf::scene::add_front(controller->get_subsurface_root(), node);
    sync();
}

void preview::sync()
{
    if (node && node->parent())
    {
        wf::scene::set_node_enabled(node, subsurface->surface->mapped);
        node->refresh();
    }
}

void preview::close()
{
    on_parent_commit.disconnect();
    on_commit.disconnect();
    on_map.disconnect();
    on_unmap.disconnect();
    on_subsurface_destroy.disconnect();
    on_unmapped.disconnect();
    if (node && node->parent())
    {
        wf::scene::remove_child(node);
    }

    node.reset();
}
}
