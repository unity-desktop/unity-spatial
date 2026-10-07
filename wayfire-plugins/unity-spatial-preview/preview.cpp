/* preview.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "preview.hpp"

#include <wayfire/scene-operations.hpp>

namespace unity_spatial_preview
{
preview::preview(wlr_surface *surface, wayfire_view view, std::function<void()> closed) :
    surface(surface), closed(std::move(closed))
{
    on_surface_destroy.set_callback([this] (void*)
    {
        detach();
        on_commit.disconnect();
        on_surface_destroy.disconnect();
        this->surface = nullptr;
    });
    on_surface_destroy.connect(&surface->events.destroy);

    if (!view || !view->is_mapped())
    {
        this->closed();
        return;
    }

    node = std::make_shared<preview_node_t>(view, surface);
    on_commit.set_callback([this] (void*)
    {
        commit();
    });
    on_commit.connect(&surface->events.commit);
    on_unmapped = [this] (auto)
    {
        close();
    };
    view->connect(&on_unmapped);
    commit();
}

preview::~preview()
{
    detach();
}

void preview::set_alpha(float alpha)
{
    pending_alpha = alpha;
}

void preview::commit()
{
    if (!node || !surface)
    {
        return;
    }

    attach();
    wf::scene::damage_node(node, node->get_bounding_box());
    node->alpha = pending_alpha;
}

void preview::attach()
{
    auto *sub = wlr_subsurface_try_from_wlr_surface(surface);
    if (!root.expired() || !sub || !sub->data)
    {
        return;
    }

    auto sub_root = static_cast<wf::wlr_subsurface_controller_t*>(sub->data)->get_subsurface_root();
    root = sub_root;
    wf::scene::add_front(sub_root, node);
    LOGD("unity-spatial-preview: attached");
    on_subsurface_destroy.set_callback([this] (void*)
    {
        detach();
    });
    on_subsurface_destroy.connect(&sub->events.destroy);
}

void preview::detach()
{
    on_subsurface_destroy.disconnect();
    auto sub_root = root.lock();
    if (sub_root && node)
    {
        wf::scene::remove_child(node);
        LOGD("unity-spatial-preview: detached");
    }

    root.reset();
}

void preview::close()
{
    detach();
    on_commit.disconnect();
    on_unmapped.disconnect();
    node.reset();
    LOGD("unity-spatial-preview: closed");
    closed();
}
}
