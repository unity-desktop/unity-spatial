/* unity-spatial-workspace-page.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WORKSPACE_PAGE (unity_spatial_workspace_page_get_type ())

/**
 * UnitySpatialWorkspacePage:
 *
 * One workspace of the grid of an output.
 */
G_DECLARE_FINAL_TYPE (UnitySpatialWorkspacePage, unity_spatial_workspace_page, UNITY_SPATIAL, WORKSPACE_PAGE, GObject)

/**
 * unity_spatial_workspace_page_get_x:
 * @self: a #UnitySpatialWorkspacePage
 *
 * Returns: the column of the workspace
 */
gint unity_spatial_workspace_page_get_x (UnitySpatialWorkspacePage *self);

/**
 * unity_spatial_workspace_page_get_y:
 * @self: a #UnitySpatialWorkspacePage
 *
 * Returns: the row of the workspace
 */
gint unity_spatial_workspace_page_get_y (UnitySpatialWorkspacePage *self);

G_END_DECLS
