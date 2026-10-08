/* unity-spatial-window-view-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-window-page-private.h"
#include "unity-spatial-window-view.h"

G_BEGIN_DECLS

void unity_spatial_window_view_update (UnitySpatialWindowView *self,
                                       GArray                 *windows);

G_END_DECLS
