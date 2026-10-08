/* protocol.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocol.hpp"

#include <wayfire/plugins/ipc/ipc-helpers.hpp>

#include "preview.hpp"
#include "unity-spatial-preview-unstable-protocol.h"

namespace unity_spatial_preview
{
namespace
{
preview *from_resource(wl_resource *resource)
{
    return static_cast<preview*>(wl_resource_get_user_data(resource));
}

void handle_destroy(wl_client*, wl_resource *resource)
{
    wl_resource_destroy(resource);
}

const struct zunity_preview_interface preview_impl = {
    .destroy = handle_destroy,
};

void destroy_preview(wl_resource *resource)
{
    delete from_resource(resource);
}

void handle_get_preview(wl_client *client, wl_resource *manager, uint32_t id, wl_resource *surface_resource,
    uint32_t view_id)
{
    auto *subsurface = wlr_subsurface_try_from_wlr_surface(wlr_surface_from_resource(surface_resource));
    if (!subsurface)
    {
        wl_resource_post_error(manager, ZUNITY_PREVIEW_MANAGER_ERROR_INVALID_SURFACE,
            "the surface is not a subsurface");
        return;
    }

    auto *resource = wl_resource_create(client, &zunity_preview_interface,
        wl_resource_get_version(manager), id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    auto view = wf::ipc::find_view_by_id(view_id);
    auto *target = (view && view->is_mapped()) ? new preview(subsurface, view) : nullptr;
    wl_resource_set_implementation(resource, &preview_impl, target, destroy_preview);
}

const struct zunity_preview_manager_interface manager_impl = {
    .get_preview = handle_get_preview,
    .destroy     = handle_destroy,
};

void bind_manager(wl_client *client, void*, uint32_t version, uint32_t id)
{
    auto *resource = wl_resource_create(client, &zunity_preview_manager_interface, version, id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &manager_impl, nullptr, nullptr);
}
}

protocol::protocol(wl_display *display) :
    global(wl_global_create(display, &zunity_preview_manager_interface, 1, nullptr, bind_manager),
        wl_global_destroy)
{}
}
