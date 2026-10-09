/* unity-spatial-wayfire-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <libdex.h>

#include "unity-spatial-window-view.h"
#include "unity-spatial-workspace-view.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WAYFIRE (unity_spatial_wayfire_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWayfire, unity_spatial_wayfire, UNITY_SPATIAL, WAYFIRE, GObject)

UnitySpatialWayfire       *unity_spatial_wayfire_get_default        (void);

UnitySpatialWindowView    *unity_spatial_wayfire_get_window_view    (UnitySpatialWayfire *self);

UnitySpatialWorkspaceView *unity_spatial_wayfire_get_workspace_view (UnitySpatialWayfire *self);

void                       unity_spatial_wayfire_refresh            (UnitySpatialWayfire *self);

void                       unity_spatial_wayfire_focus_view         (UnitySpatialWayfire *self,
                                                                     guint                view_id);

void                       unity_spatial_wayfire_close_view         (UnitySpatialWayfire *self,
                                                                     guint                view_id);

void                       unity_spatial_wayfire_send_view          (UnitySpatialWayfire *self,
                                                                     guint                view_id,
                                                                     gint                 x,
                                                                     gint                 y);

DexFuture                 *unity_spatial_wayfire_set_workspace      (UnitySpatialWayfire *self,
                                                                     gint                 x,
                                                                     gint                 y);

G_END_DECLS
