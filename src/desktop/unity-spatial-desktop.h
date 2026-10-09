/* unity-spatial-desktop.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-view.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_DESKTOP (unity_spatial_desktop_get_type ())

/**
 * UnitySpatialDesktop:
 *
 * The desktop: one wallpaper window on the background layer of each monitor, and the
 * spatial view of the windows, which springs out of the real windows when it opens.
 */
G_DECLARE_FINAL_TYPE (UnitySpatialDesktop, unity_spatial_desktop, UNITY_SPATIAL, DESKTOP, GObject)

/**
 * unity_spatial_desktop_new:
 * @app: the #GtkApplication the surfaces belong to
 *
 * Creates the desktop. It follows the monitors and the wallpaper settings.
 *
 * Returns: (transfer full): a new #UnitySpatialDesktop
 */
UnitySpatialDesktop *unity_spatial_desktop_new      (GtkApplication      *app);

/**
 * unity_spatial_desktop_get_page:
 * @self: a #UnitySpatialDesktop
 *
 * Returns: the page that shows or that the desktop animates to
 */
UnitySpatialPage     unity_spatial_desktop_get_page (UnitySpatialDesktop *self);

/**
 * unity_spatial_desktop_set_page:
 * @self: a #UnitySpatialDesktop
 * @page: the page to show
 *
 * Animates to @page. The windows and workspaces pages show in the work area of the
 * monitor whose windows the window view lists, so panels and docks stay usable.
 */
void                 unity_spatial_desktop_set_page (UnitySpatialDesktop *self,
                                                     UnitySpatialPage     page);

/**
 * unity_spatial_desktop_get_view_visible:
 * @self: a #UnitySpatialDesktop
 *
 * The spatial view shows from the start of an open, by page or by gesture, until the
 * end of its close. It is on the overlay layer, so it shows above fullscreen
 * windows; shell surfaces that must stay usable with it can follow this.
 *
 * Returns: whether the spatial view shows
 */
gboolean             unity_spatial_desktop_get_view_visible (UnitySpatialDesktop *self);

G_END_DECLS
