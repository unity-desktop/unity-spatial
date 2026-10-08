/* ipc.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wayfire/plugins/common/shared-core-data.hpp>
#include <wayfire/plugins/ipc/ipc-method-repository.hpp>

namespace unity_spatial_preview
{
class ipc_methods
{
  public:
    ipc_methods();
    ~ipc_methods();

    ipc_methods(const ipc_methods&) = delete;
    ipc_methods& operator =(const ipc_methods&) = delete;

  private:
    wf::shared_data::ref_ptr_t<wf::ipc::method_repository_t> repository;
};
}
