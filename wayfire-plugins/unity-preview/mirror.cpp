/* mirror.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mirror.hpp"
#include "mirror-node.hpp"

#include <algorithm>

#include <wayfire/core.hpp>
#include <wayfire/dassert.hpp>
#include <wayfire/scene-operations.hpp>

#include "unity-preview-v1-protocol.h"

namespace unity_preview
{
namespace
{
struct mirror_root_t : public wf::custom_data_t
{
    wf::scene::floating_inner_ptr node = std::make_shared<wf::scene::floating_inner_node_t>(false);
};

wf::scene::floating_inner_ptr get_mirror_root(wayfire_view host)
{
    auto root = host->get_data_safe<mirror_root_t>();
    if (!root->node->parent())
    {
        wf::scene::add_front(host->get_surface_root_node(), root->node);
    }

    return root->node;
}

int32_t order_of(const wf::scene::node_ptr& node)
{
    auto *mirror_node = dynamic_cast<mirror_node_t*>(node.get());
    wf::dassert(mirror_node, "unity-preview: a mirror root holds only mirror nodes");
    return mirror_node->order;
}
}

class mirror::attachment
{
  public:
    attachment(const preview_state& state, wayfire_view source, wayfire_view host_view, wlr_surface *host) :
        state(state), mirror_root(get_mirror_root(host_view)),
        node(std::make_shared<mirror_node_t>(source, host))
    {
        reconcile();
        wf::scene::add_front(mirror_root, node);
        restack();
    }

    ~attachment()
    {
        wf::scene::remove_child(node);
    }

    attachment(const attachment&) = delete;
    attachment& operator =(const attachment&) = delete;

    void reconcile()
    {
        wf::scene::damage_node(node, node->get_bounding_box());
        node->rect  = state.rect;
        node->alpha = state.alpha;
        if (node->order != state.order)
        {
            node->order = state.order;
            restack();
        }

        set_enabled((state.rect.width > 0) && (state.rect.height > 0) && (state.alpha > 0.0f));
        wf::scene::damage_node(node, node->get_bounding_box());
    }

  private:
    void restack()
    {
        auto children = mirror_root->get_children();
        std::stable_sort(children.begin(), children.end(), [] (const auto& a, const auto& b)
        {
            return order_of(a) > order_of(b);
        });
        mirror_root->set_children_list(children);
        wf::scene::update(mirror_root, wf::scene::update_flag::CHILDREN_LIST);
    }

    void set_enabled(bool enabled)
    {
        if (enabled != node->is_enabled())
        {
            wf::scene::set_node_enabled(node, enabled);
        }
    }

    const preview_state& state;
    wf::scene::floating_inner_ptr mirror_root;
    std::shared_ptr<mirror_node_t> node;
};

const wlr_surface_synced_impl mirror::synced_impl = {
    .state_size   = sizeof(preview_state),
    .init_state   = mirror::init_state,
    .finish_state = nullptr,
    .move_state   = nullptr,
    .commit = mirror::commit_state,
};

mirror::mirror(wl_resource *resource, wayfire_view source, wl_resource *surface) : resource(resource)
{
    if (!source || !source->is_mapped())
    {
        unity_preview_mirror_v1_send_closed(resource);
        return;
    }

    link.owner = this;
    if (!wlr_surface_synced_init(&link.synced, wlr_surface_from_resource(surface), &synced_impl, &pending,
        &current))
    {
        wl_resource_post_no_memory(resource);
        return;
    }

    host = wlr_surface_from_resource(surface);
    this->source = source->weak_from_this();

    on_host_destroy.set_callback([this] (void*)
    {
        release_host();
    });
    on_host_destroy.connect(&host->events.destroy);

    on_view_mapped = [this] (wf::view_mapped_signal *ev)
    {
        if (ev->view->get_wlr_surface() == host)
        {
            attach(ev->view);
        }
    };
    on_host_unmapped = [this] (auto)
    {
        detach();
    };
    on_source_unmapped = [this] (auto)
    {
        close();
    };
    on_source_geometry_changed = [this] (auto)
    {
        send_source_size();
    };

    wf::get_core().connect(&on_view_mapped);
    source->connect(&on_source_unmapped);
    source->connect(&on_source_geometry_changed);
    send_source_size();

    auto host_view = wf::wl_surface_to_wayfire_view(surface);
    if (host_view && host_view->is_mapped())
    {
        attach(host_view);
    }
}

mirror::~mirror()
{
    release_host();
}

void mirror::init_state(void *state)
{
    *static_cast<preview_state*>(state) = {};
}

void mirror::commit_state(wlr_surface_synced *synced)
{
    auto *self = reinterpret_cast<synced_link*>(synced)->owner;
    if (self->attached)
    {
        self->attached->reconcile();
    }
}

void mirror::set_rect(const wf::geometry_t& rect)
{
    pending.rect = rect;
}

void mirror::set_alpha(float alpha)
{
    pending.alpha = alpha;
}

void mirror::set_order(int32_t order)
{
    pending.order = order;
}

void mirror::attach(wayfire_view host_view)
{
    auto view = source.lock();
    wf::dassert(host != nullptr, "unity-preview: attach without a host surface");
    if (attached || !view)
    {
        return;
    }

    attached = std::make_unique<attachment>(current, view.get(), host_view, host);
    host_view->connect(&on_host_unmapped);
}

void mirror::detach()
{
    on_host_unmapped.disconnect();
    attached.reset();
}

void mirror::close()
{
    detach();
    on_view_mapped.disconnect();
    on_source_unmapped.disconnect();
    on_source_geometry_changed.disconnect();
    source.reset();
    unity_preview_mirror_v1_send_closed(resource);
}

void mirror::release_host()
{
    if (!host)
    {
        return;
    }

    detach();
    on_view_mapped.disconnect();
    on_host_destroy.disconnect();
    wlr_surface_synced_finish(&link.synced);
    host = nullptr;
}

void mirror::send_source_size()
{
    auto view = source.lock();
    if (!view)
    {
        return;
    }

    auto size = wf::dimensions(view->get_surface_root_node()->get_bounding_box());
    if (size != source_size)
    {
        source_size = size;
        unity_preview_mirror_v1_send_source_size(resource, size.width, size.height);
    }
}
}
