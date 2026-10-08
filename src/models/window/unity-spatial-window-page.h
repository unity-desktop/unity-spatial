/* unity-spatial-window-page.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WINDOW_PAGE (unity_spatial_window_page_get_type ())

/**
 * UnitySpatialWindowPage:
 *
 * One window of the desktop, as the overview shows it.
 */
G_DECLARE_FINAL_TYPE (UnitySpatialWindowPage, unity_spatial_window_page, UNITY_SPATIAL, WINDOW_PAGE, GObject)

/**
 * unity_spatial_window_page_get_view_id:
 * @self: a #UnitySpatialWindowPage
 *
 * Returns: the wayfire view id of the window
 */
guint               unity_spatial_window_page_get_view_id     (UnitySpatialWindowPage *self);

/**
 * unity_spatial_window_page_get_app_id:
 * @self: a #UnitySpatialWindowPage
 *
 * Returns: (nullable): the app id of the window
 */
const gchar        *unity_spatial_window_page_get_app_id      (UnitySpatialWindowPage *self);

/**
 * unity_spatial_window_page_get_title:
 * @self: a #UnitySpatialWindowPage
 *
 * Returns: (nullable): the title of the window
 */
const gchar        *unity_spatial_window_page_get_title       (UnitySpatialWindowPage *self);

/**
 * unity_spatial_window_page_get_bounds:
 * @self: a #UnitySpatialWindowPage
 *
 * Gets the window's frame without its client-side shadow.
 *
 * Returns: (transfer none): the bounds, relative to the window's own workspace
 */
const GdkRectangle *unity_spatial_window_page_get_bounds      (UnitySpatialWindowPage *self);

/**
 * unity_spatial_window_page_get_workspace_x:
 * @self: a #UnitySpatialWindowPage
 *
 * Returns: the column of the window's workspace
 */
gint                unity_spatial_window_page_get_workspace_x (UnitySpatialWindowPage *self);

/**
 * unity_spatial_window_page_get_workspace_y:
 * @self: a #UnitySpatialWindowPage
 *
 * Returns: the row of the window's workspace
 */
gint                unity_spatial_window_page_get_workspace_y (UnitySpatialWindowPage *self);

/**
 * unity_spatial_window_page_get_minimized:
 * @self: a #UnitySpatialWindowPage
 *
 * Returns: %TRUE if the window is minimized
 */
gboolean            unity_spatial_window_page_get_minimized   (UnitySpatialWindowPage *self);

G_END_DECLS
