/* unity-spatial-window-grid-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WINDOW_GRID (unity_spatial_window_grid_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWindowGrid, unity_spatial_window_grid, UNITY_SPATIAL, WINDOW_GRID, GtkWidget)

void       unity_spatial_window_grid_set_model (UnitySpatialWindowGrid *self,
                                                GListModel             *model);

void       unity_spatial_window_grid_set_morph (UnitySpatialWindowGrid *self,
                                                gdouble                 morph);

void       unity_spatial_window_grid_set_wall  (UnitySpatialWindowGrid *self,
                                                gboolean                wall);

G_END_DECLS
