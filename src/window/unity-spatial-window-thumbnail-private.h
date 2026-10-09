/* unity-spatial-window-thumbnail-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "unity-spatial-window-page.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WINDOW_THUMBNAIL (unity_spatial_window_thumbnail_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWindowThumbnail, unity_spatial_window_thumbnail, UNITY_SPATIAL, WINDOW_THUMBNAIL, GtkWidget)

UnitySpatialWindowThumbnail *unity_spatial_window_thumbnail_new         (UnitySpatialWindowPage      *page);

UnitySpatialWindowPage      *unity_spatial_window_thumbnail_get_page    (UnitySpatialWindowThumbnail *self);

G_END_DECLS
