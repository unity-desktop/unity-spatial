/* plugin.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>

#include <wayfire/core.hpp>
#include <wayfire/plugin.hpp>

#include "protocol.hpp"
#include "stacking.hpp"

namespace unity_spatial_preview
{
class plugin : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        wayland = std::make_unique<protocol>(wf::get_core().display);
        ipc     = std::make_unique<stacking_ipc>();
    }

    void fini() override
    {
        ipc.reset();
        wayland.reset();
    }

    bool is_unloadable() override
    {
        return false;
    }

  private:
    std::unique_ptr<protocol> wayland;
    std::unique_ptr<stacking_ipc> ipc;
};
}

DECLARE_WAYFIRE_PLUGIN(unity_spatial_preview::plugin);
