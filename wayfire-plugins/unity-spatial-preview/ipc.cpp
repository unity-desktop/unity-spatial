/* ipc.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ipc.hpp"

#include <wayfire/output.hpp>
#include <wayfire/workspace-set.hpp>
#include <wayfire/plugins/ipc/ipc-helpers.hpp>

namespace unity_spatial_preview
{
namespace
{
constexpr const char *STACKING      = "unity-spatial-preview/stacking";
constexpr const char *SET_WORKSPACE = "unity-spatial-preview/set-workspace";

wf::output_t *output_of(const wf::json_t& data)
{
    return wf::ipc::find_output_by_id(wf::ipc::json_get_uint64(data, "output-id"));
}

wf::json_t stacking(const wf::json_t& data)
{
    auto output = output_of(data);
    if (!output)
    {
        return wf::ipc::json_error("output id not found!");
    }

    auto response = wf::ipc::json_ok();
    response["views"] = wf::json_t::array();
    for (auto& view : output->wset()->get_views(wf::WSET_MAPPED_ONLY | wf::WSET_SORT_STACKING))
    {
        response["views"].append(view->get_id());
    }

    return response;
}

wf::json_t set_workspace(const wf::json_t& data)
{
    auto output = output_of(data);
    if (!output)
    {
        return wf::ipc::json_error("output id not found!");
    }

    wf::point_t workspace = {int(wf::ipc::json_get_uint64(data, "x")), int(wf::ipc::json_get_uint64(data, "y"))};
    auto grid = output->wset()->get_workspace_grid_size();
    if ((workspace.x >= grid.width) || (workspace.y >= grid.height))
    {
        return wf::ipc::json_error("workspace out of the grid!");
    }

    output->wset()->set_workspace(workspace);
    return wf::ipc::json_ok();
}
}

ipc_methods::ipc_methods()
{
    repository->register_method(STACKING, stacking);
    repository->register_method(SET_WORKSPACE, set_workspace);
}

ipc_methods::~ipc_methods()
{
    repository->unregister_method(SET_WORKSPACE);
    repository->unregister_method(STACKING);
}
}
