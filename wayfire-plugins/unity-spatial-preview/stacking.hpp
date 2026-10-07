/* stacking.hpp
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
class stacking_ipc
{
  public:
    stacking_ipc();
    ~stacking_ipc();

    stacking_ipc(const stacking_ipc&) = delete;
    stacking_ipc& operator =(const stacking_ipc&) = delete;

  private:
    wf::shared_data::ref_ptr_t<wf::ipc::method_repository_t> repository;
};
}
