/* unity-spatial-window-layout-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WINDOW_LAYOUT (unity_spatial_window_layout_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWindowLayout, unity_spatial_window_layout, UNITY_SPATIAL, WINDOW_LAYOUT, GtkLayoutManager)

void unity_spatial_window_layout_set_morph (UnitySpatialWindowLayout *self,
                                            gdouble                   morph);

void unity_spatial_window_layout_fly_from  (UnitySpatialWindowLayout *self,
                                            GtkWidget                *child,
                                            gdouble                   dx,
                                            gdouble                   dy);

G_END_DECLS
