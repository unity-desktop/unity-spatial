/* unity-spatial-view.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * UnitySpatialPage:
 * @UNITY_SPATIAL_PAGE_DESKTOP: the wallpaper under the real windows
 * @UNITY_SPATIAL_PAGE_WINDOWS: the windows of the current workspace in a grid
 * @UNITY_SPATIAL_PAGE_WORKSPACES: all workspaces of the output in their grid
 *
 * The pages of a [class@View].
 */
typedef enum
{
  UNITY_SPATIAL_PAGE_DESKTOP,
  UNITY_SPATIAL_PAGE_WINDOWS,
  UNITY_SPATIAL_PAGE_WORKSPACES,
} UnitySpatialPage;

#define UNITY_SPATIAL_TYPE_VIEW (unity_spatial_view_get_type ())

/**
 * UnitySpatialView:
 *
 * The workspaces of the output in a carousel, below a search bar. It shows one
 * of three pages: the desktop (each window where it is), the windows (the
 * windows of the current workspace in a grid) and the workspaces (every
 * workspace in its grid). Between two pages it morphs from one to the other.
 */
G_DECLARE_FINAL_TYPE (UnitySpatialView, unity_spatial_view, UNITY_SPATIAL, VIEW, GtkWidget)

/**
 * unity_spatial_view_new:
 *
 * Returns: (transfer full): a new #UnitySpatialView
 */
GtkWidget        *unity_spatial_view_new               (void);

/**
 * unity_spatial_view_get_page:
 * @self: a #UnitySpatialView
 *
 * Returns: the page that shows or that @self animates to
 */
UnitySpatialPage  unity_spatial_view_get_page          (UnitySpatialView *self);

/**
 * unity_spatial_view_set_page:
 * @self: a #UnitySpatialView
 * @page: the page to show
 *
 * Animates to @page.
 */
void              unity_spatial_view_set_page          (UnitySpatialView *self,
                                                        UnitySpatialPage  page);

/**
 * unity_spatial_view_get_progress:
 * @self: a #UnitySpatialView
 *
 * Returns: the position between the pages, from 0 to 2
 */
gdouble           unity_spatial_view_get_progress      (UnitySpatialView *self);

/**
 * unity_spatial_view_set_progress:
 * @self: a #UnitySpatialView
 * @progress: the position between the pages, from 0 to 2
 *
 * Shows @self at @progress at once, without an animation.
 */
void              unity_spatial_view_set_progress      (UnitySpatialView *self,
                                                        gdouble           progress);

/**
 * unity_spatial_view_get_enable_search:
 * @self: a #UnitySpatialView
 *
 * Returns: whether the search bar and the search page are available
 */
gboolean          unity_spatial_view_get_enable_search (UnitySpatialView *self);

/**
 * unity_spatial_view_set_enable_search:
 * @self: a #UnitySpatialView
 * @enable_search: whether the search bar and the search page are available
 *
 * Shows or removes the search bar and the search page.
 */
void              unity_spatial_view_set_enable_search (UnitySpatialView *self,
                                                        gboolean          enable_search);

G_END_DECLS
