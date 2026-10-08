/* unity-spatial-window-view.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-window-page.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WINDOW_VIEW (unity_spatial_window_view_get_type ())

/**
 * UnitySpatialWindowView:
 *
 * The windows of the output as a [iface@Gio.ListModel] of [class@WindowPage],
 * on all its workspaces (see [class@WorkspaceView]).
 */
G_DECLARE_FINAL_TYPE (UnitySpatialWindowView, unity_spatial_window_view, UNITY_SPATIAL, WINDOW_VIEW, GObject)

/**
 * unity_spatial_window_view_get_default:
 *
 * Returns: (transfer none): the shared window view
 */
UnitySpatialWindowView *unity_spatial_window_view_get_default       (void);

/**
 * unity_spatial_window_view_activate_page:
 * @self: a #UnitySpatialWindowView
 * @page: a #UnitySpatialWindowPage of @self
 *
 * Asks the compositor to focus the window, and moves @page to the front.
 */
void                    unity_spatial_window_view_activate_page     (UnitySpatialWindowView *self,
                                                                     UnitySpatialWindowPage *page);

/**
 * unity_spatial_window_view_close_page:
 * @self: a #UnitySpatialWindowView
 * @page: a #UnitySpatialWindowPage of @self
 *
 * Asks the compositor to close the window. The page leaves the list when the
 * window is gone.
 */
void                    unity_spatial_window_view_close_page        (UnitySpatialWindowView *self,
                                                                     UnitySpatialWindowPage *page);

/**
 * unity_spatial_window_view_move_page:
 * @self: a #UnitySpatialWindowView
 * @page: a #UnitySpatialWindowPage of @self
 * @workspace_x: the target column
 * @workspace_y: the target row
 *
 * Asks the compositor to move the window to another workspace. The window keeps
 * its place on the workspace.
 */
void                    unity_spatial_window_view_move_page         (UnitySpatialWindowView *self,
                                                                     UnitySpatialWindowPage *page,
                                                                     gint                    workspace_x,
                                                                     gint                    workspace_y);

G_END_DECLS
