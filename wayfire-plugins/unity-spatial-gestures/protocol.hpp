/* protocol.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <wayland-server-core.h>

namespace unity_spatial_gestures
{
enum class kind : uint32_t
{
    swipe,
    pinch,
};

class protocol
{
  public:
    explicit protocol(wl_display *display);
    ~protocol();

    protocol(const protocol&) = delete;
    protocol& operator =(const protocol&) = delete;

    bool wants(kind gesture, uint32_t fingers) const;
    void begin(kind gesture, uint32_t fingers, uint32_t time);
    void update(kind gesture, uint32_t fingers, uint32_t time, double dx, double dy, double scale,
        double rotation);
    void end(kind gesture, uint32_t fingers, uint32_t time, bool cancelled);

  private:
    struct subscription
    {
        wl_resource *resource;
        kind gesture;
        uint32_t fingers;
    };

    std::vector<wl_resource*> matching(kind gesture, uint32_t fingers) const;

    static void bind(wl_client *client, void *data, uint32_t version, uint32_t id);
    static void get_gesture(wl_client *client, wl_resource *manager, uint32_t id, uint32_t gesture,
        uint32_t fingers);
    static void unsubscribe(wl_resource *resource);

    std::unique_ptr<wl_global, decltype(&wl_global_destroy)> global;
    std::vector<subscription> subscriptions;
};
}
