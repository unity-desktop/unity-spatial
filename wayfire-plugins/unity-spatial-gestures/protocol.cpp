/* protocol.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocol.hpp"

#include <algorithm>

#include "unity-spatial-gestures-v1-protocol.h"

namespace unity_spatial_gestures
{
namespace
{
constexpr uint32_t MIN_FINGERS = 3;

static_assert(uint32_t(kind::swipe) == UNITY_SPATIAL_GESTURES_V1_KIND_SWIPE);
static_assert(uint32_t(kind::pinch) == UNITY_SPATIAL_GESTURES_V1_KIND_PINCH);

void handle_destroy(wl_client*, wl_resource *resource)
{
    wl_resource_destroy(resource);
}

const struct unity_spatial_gesture_v1_interface gesture_impl = {
    .destroy = handle_destroy,
};
}

protocol::protocol(wl_display *display) :
    global(wl_global_create(display, &unity_spatial_gestures_v1_interface, 1, this, bind), wl_global_destroy)
{}

protocol::~protocol()
{
    for (auto& sub : subscriptions)
    {
        wl_resource_set_user_data(sub.resource, nullptr);
    }
}

bool protocol::wants(kind gesture, uint32_t fingers) const
{
    return !matching(gesture, fingers).empty();
}

void protocol::begin(kind gesture, uint32_t fingers, uint32_t time)
{
    for (auto *resource : matching(gesture, fingers))
    {
        unity_spatial_gesture_v1_send_begin(resource, time);
    }
}

void protocol::update(kind gesture, uint32_t fingers, uint32_t time, double dx, double dy, double scale,
    double rotation)
{
    for (auto *resource : matching(gesture, fingers))
    {
        unity_spatial_gesture_v1_send_update(resource, time, wl_fixed_from_double(dx), wl_fixed_from_double(dy),
            wl_fixed_from_double(scale), wl_fixed_from_double(rotation));
    }
}

void protocol::end(kind gesture, uint32_t fingers, uint32_t time, bool cancelled)
{
    for (auto *resource : matching(gesture, fingers))
    {
        unity_spatial_gesture_v1_send_end(resource, time, cancelled);
    }
}

std::vector<wl_resource*> protocol::matching(kind gesture, uint32_t fingers) const
{
    std::vector<wl_resource*> resources;
    for (auto& sub : subscriptions)
    {
        if ((sub.gesture == gesture) && (sub.fingers == fingers))
        {
            resources.push_back(sub.resource);
        }
    }

    return resources;
}

void protocol::bind(wl_client *client, void *data, uint32_t version, uint32_t id)
{
    static const struct unity_spatial_gestures_v1_interface gestures_impl = {
        .destroy     = handle_destroy,
        .get_gesture = get_gesture,
    };

    auto *resource = wl_resource_create(client, &unity_spatial_gestures_v1_interface, version, id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &gestures_impl, data, nullptr);
}

void protocol::get_gesture(wl_client *client, wl_resource *manager, uint32_t id, uint32_t gesture,
    uint32_t fingers)
{
    if (gesture > uint32_t(kind::pinch))
    {
        wl_resource_post_error(manager, UNITY_SPATIAL_GESTURES_V1_ERROR_INVALID_KIND, "unknown kind %u", gesture);
        return;
    }

    if (fingers < MIN_FINGERS)
    {
        wl_resource_post_error(manager, UNITY_SPATIAL_GESTURES_V1_ERROR_INVALID_FINGERS,
            "fingers must be 3 or more");
        return;
    }

    auto *resource = wl_resource_create(client, &unity_spatial_gesture_v1_interface,
        wl_resource_get_version(manager), id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    auto *self = static_cast<protocol*>(wl_resource_get_user_data(manager));
    wl_resource_set_implementation(resource, &gesture_impl, self, unsubscribe);
    self->subscriptions.push_back({resource, kind(gesture), fingers});
}

void protocol::unsubscribe(wl_resource *resource)
{
    if (auto *self = static_cast<protocol*>(wl_resource_get_user_data(resource)))
    {
        auto& list = self->subscriptions;
        list.erase(std::remove_if(list.begin(), list.end(), [&] (const subscription& sub)
        {
            return sub.resource == resource;
        }), list.end());
    }
}
}
