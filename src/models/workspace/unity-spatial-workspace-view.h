/* unity-spatial-workspace-view.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gdk/gdk.h>

#include "unity-spatial-workspace-page.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WORKSPACE_VIEW (unity_spatial_workspace_view_get_type ())

/**
 * UnitySpatialWorkspaceView:
 *
 * The workspaces of the output as a [iface@Gio.ListModel] of [class@WorkspacePage],
 * row by row, with the output they belong to.
 */
G_DECLARE_FINAL_TYPE (UnitySpatialWorkspaceView, unity_spatial_workspace_view, UNITY_SPATIAL, WORKSPACE_VIEW, GObject)

/**
 * unity_spatial_workspace_view_get_default:
 *
 * Returns: (transfer none): the shared workspace view
 */
UnitySpatialWorkspaceView *unity_spatial_workspace_view_get_default     (void);

/**
 * unity_spatial_workspace_view_get_workspace:
 * @self: a #UnitySpatialWorkspaceView
 * @x: the column
 * @y: the row
 *
 * Returns: (transfer none) (nullable): the workspace at @x, @y, or %NULL
 *   outside the grid
 */
UnitySpatialWorkspacePage *unity_spatial_workspace_view_get_workspace   (UnitySpatialWorkspaceView *self,
                                                                         gint                       x,
                                                                         gint                       y);

/**
 * unity_spatial_workspace_view_get_current:
 * @self: a #UnitySpatialWorkspaceView
 *
 * Returns: (transfer none) (nullable): the workspace that the output shows
 */
UnitySpatialWorkspacePage *unity_spatial_workspace_view_get_current     (UnitySpatialWorkspaceView *self);

/**
 * unity_spatial_workspace_view_get_grid_width:
 * @self: a #UnitySpatialWorkspaceView
 *
 * Returns: the number of workspace columns
 */
gint                       unity_spatial_workspace_view_get_grid_width  (UnitySpatialWorkspaceView *self);

/**
 * unity_spatial_workspace_view_get_grid_height:
 * @self: a #UnitySpatialWorkspaceView
 *
 * Returns: the number of workspace rows
 */
gint                       unity_spatial_workspace_view_get_grid_height (UnitySpatialWorkspaceView *self);

/**
 * unity_spatial_workspace_view_get_output_name:
 * @self: a #UnitySpatialWorkspaceView
 *
 * Returns: (nullable): the connector name of the output, such as eDP-1
 */
const gchar               *unity_spatial_workspace_view_get_output_name (UnitySpatialWorkspaceView *self);

/**
 * unity_spatial_workspace_view_get_output_size:
 * @self: a #UnitySpatialWorkspaceView
 * @width: (out): the logical width of the output
 * @height: (out): the logical height of the output
 */
void                       unity_spatial_workspace_view_get_output_size (UnitySpatialWorkspaceView *self,
                                                                         gint                      *width,
                                                                         gint                      *height);

/**
 * unity_spatial_workspace_view_get_workarea:
 * @self: a #UnitySpatialWorkspaceView
 *
 * Returns: (transfer none): the part of the output that panels and docks leave
 *   free, in output coordinates
 */
const GdkRectangle        *unity_spatial_workspace_view_get_workarea    (UnitySpatialWorkspaceView *self);

/**
 * unity_spatial_workspace_view_get_desktop_view_id:
 * @self: a #UnitySpatialWorkspaceView
 *
 * Returns: the wayfire view id of the desktop surface (namespace `unity-desktop`)
 *   on the output, or 0
 */
guint                      unity_spatial_workspace_view_get_desktop_view_id (UnitySpatialWorkspaceView *self);

/**
 * unity_spatial_workspace_view_activate:
 * @self: a #UnitySpatialWorkspaceView
 * @workspace: the workspace to show
 *
 * Asks the compositor to show @workspace on the output.
 */
void                       unity_spatial_workspace_view_activate        (UnitySpatialWorkspaceView *self,
                                                                         UnitySpatialWorkspacePage *workspace);

G_END_DECLS
