/* unity-spatial-view-window-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unity-window.h>

#include "unity-spatial-view-private.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_VIEW_WINDOW (unity_spatial_view_window_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialViewWindow, unity_spatial_view_window, UNITY_SPATIAL, VIEW_WINDOW, UnityWindow)

UnitySpatialViewWindow *unity_spatial_view_window_new      (GtkApplication         *app,
                                                            GdkMonitor             *monitor);

UnitySpatialView       *unity_spatial_view_window_get_view (UnitySpatialViewWindow *self);

G_END_DECLS
