/* unity-spatial-wallpaper-window-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unity-backgrounds-source.h>
#include <unity-window.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WALLPAPER_WINDOW (unity_spatial_wallpaper_window_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWallpaperWindow, unity_spatial_wallpaper_window, UNITY_SPATIAL, WALLPAPER_WINDOW, UnityWindow)

UnitySpatialWallpaperWindow *unity_spatial_wallpaper_window_new (GtkApplication         *app,
                                                               GdkMonitor             *monitor,
                                                               UnityBackgroundsSource *source);

G_END_DECLS
