/* plugin.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mirror.hpp"

#include <memory>

#include <wayfire/core.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/plugins/ipc/ipc-helpers.hpp>

#include "unity-preview-v1-protocol.h"

namespace unity_preview
{
namespace
{
mirror *from_resource(wl_resource *resource)
{
    return static_cast<mirror*>(wl_resource_get_user_data(resource));
}

void handle_destroy(wl_client*, wl_resource *resource)
{
    wl_resource_destroy(resource);
}

void handle_set_rect(wl_client*, wl_resource *resource, wl_fixed_t x, wl_fixed_t y,
    wl_fixed_t width, wl_fixed_t height)
{
    if ((width < 0) || (height < 0))
    {
        wl_resource_post_error(resource, UNITY_PREVIEW_MIRROR_V1_ERROR_INVALID_RECT,
            "width and height must not be negative");
        return;
    }

    from_resource(resource)->set_rect({wl_fixed_to_double(x), wl_fixed_to_double(y),
            wl_fixed_to_double(width), wl_fixed_to_double(height)});
}

void handle_set_alpha(wl_client*, wl_resource *resource, wl_fixed_t alpha)
{
    if ((alpha < 0) || (alpha > wl_fixed_from_int(1)))
    {
        wl_resource_post_error(resource, UNITY_PREVIEW_MIRROR_V1_ERROR_INVALID_ALPHA,
            "alpha must be from 0 to 1");
        return;
    }

    from_resource(resource)->set_alpha(float(wl_fixed_to_double(alpha)));
}

void handle_set_order(wl_client*, wl_resource *resource, int32_t order)
{
    from_resource(resource)->set_order(order);
}

const struct unity_preview_mirror_v1_interface mirror_impl = {
    .set_rect  = handle_set_rect,
    .set_alpha = handle_set_alpha,
    .set_order = handle_set_order,
    .destroy   = handle_destroy,
};

void destroy_mirror(wl_resource *resource)
{
    delete from_resource(resource);
}

void handle_get_mirror(wl_client *client, wl_resource *manager, uint32_t id,
    wl_resource *surface, uint32_t view_id)
{
    auto *resource = wl_resource_create(client, &unity_preview_mirror_v1_interface,
        wl_resource_get_version(manager), id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    auto source = wf::toplevel_cast(wf::ipc::find_view_by_id(view_id));
    wl_resource_set_implementation(resource, &mirror_impl, new mirror(resource, source, surface),
        destroy_mirror);
}

const struct unity_preview_manager_v1_interface manager_impl = {
    .get_mirror = handle_get_mirror,
    .destroy    = handle_destroy,
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

class plugin : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        global.reset(wl_global_create(wf::get_core().display, &unity_preview_manager_v1_interface, 1,
            nullptr, bind_manager));
    }

    void fini() override
    {
        global.reset();
    }

    bool is_unloadable() override
    {
        return false;
    }

  private:
    std::unique_ptr<wl_global, decltype(&wl_global_destroy)> global{nullptr, wl_global_destroy};
};
}

DECLARE_WAYFIRE_PLUGIN(unity_preview::plugin);
