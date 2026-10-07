/* plugin.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>

#include <wayfire/core.hpp>
#include <wayfire/plugin.hpp>

#include "capture.hpp"
#include "protocol.hpp"

namespace unity_spatial_gestures
{
class plugin : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        wayland = std::make_unique<protocol>(wf::get_core().display);
        swipes  = std::make_unique<capture>(*wayland);
    }

    void fini() override
    {
        swipes.reset();
        wayland.reset();
    }

    bool is_unloadable() override
    {
        return false;
    }

  private:
    std::unique_ptr<protocol> wayland;
    std::unique_ptr<capture> swipes;
};
}

DECLARE_WAYFIRE_PLUGIN(unity_spatial_gestures::plugin);
