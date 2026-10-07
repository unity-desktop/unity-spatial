/* protocol.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocol.hpp"

#include <wayfire/plugins/ipc/ipc-helpers.hpp>

#include "preview.hpp"
#include "unity-spatial-preview-v1-protocol.h"

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

void handle_set_alpha(wl_client*, wl_resource *resource, wl_fixed_t alpha)
{
    if ((alpha < 0) || (alpha > wl_fixed_from_int(1)))
    {
        wl_resource_post_error(resource, UNITY_PREVIEW_V1_ERROR_INVALID_ALPHA, "alpha must be from 0 to 1");
        return;
    }

    from_resource(resource)->set_alpha(float(wl_fixed_to_double(alpha)));
}

const struct unity_preview_v1_interface preview_impl = {
    .set_alpha = handle_set_alpha,
    .destroy   = handle_destroy,
};

void destroy_preview(wl_resource *resource)
{
    delete from_resource(resource);
}

void handle_get_preview(wl_client *client, wl_resource *manager, uint32_t id, wl_resource *surface,
    uint32_t view_id)
{
    auto *resource = wl_resource_create(client, &unity_preview_v1_interface,
        wl_resource_get_version(manager), id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    auto closed = [resource] { unity_preview_v1_send_closed(resource); };
    wl_resource_set_implementation(resource, &preview_impl,
        new preview(wlr_surface_from_resource(surface), wf::ipc::find_view_by_id(view_id), closed),
        destroy_preview);
}

const struct unity_preview_manager_v1_interface manager_impl = {
    .get_preview = handle_get_preview,
    .destroy     = handle_destroy,
};

void bind_manager(wl_client *client, void*, uint32_t version, uint32_t id)
{
    auto *resource = wl_resource_create(client, &unity_preview_manager_v1_interface, version, id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &manager_impl, nullptr, nullptr);
}
}

protocol::protocol(wl_display *display) :
    global(wl_global_create(display, &unity_preview_manager_v1_interface, 1, nullptr, bind_manager),
        wl_global_destroy)
{}
}
