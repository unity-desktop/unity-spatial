/* stacking.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stacking.hpp"

#include <wayfire/output.hpp>
#include <wayfire/workspace-set.hpp>
#include <wayfire/plugins/ipc/ipc-helpers.hpp>

namespace unity_spatial_preview
{
namespace
{
constexpr const char *METHOD = "unity-spatial-preview/stacking";

wf::json_t stacking(const wf::json_t& data)
{
    if (!data.has_member("output-id") || !data["output-id"].is_int())
    {
        return wf::ipc::json_error("missing output-id");
    }

    auto output = wf::ipc::find_output_by_id(data["output-id"].as_int());
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
}

stacking_ipc::stacking_ipc()
{
    repository->register_method(METHOD, stacking);
}

stacking_ipc::~stacking_ipc()
{
    repository->unregister_method(METHOD);
}
}
