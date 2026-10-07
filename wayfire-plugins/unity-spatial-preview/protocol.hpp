/* protocol.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <wayland-server-core.h>

namespace unity_spatial_preview
{
class protocol
{
  public:
    explicit protocol(wl_display *display);

  private:
    std::unique_ptr<wl_global, decltype(&wl_global_destroy)> global;
};
}
