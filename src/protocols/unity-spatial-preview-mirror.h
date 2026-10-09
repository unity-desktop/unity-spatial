/* unity-spatial-preview-mirror.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_PREVIEW_MIRROR (unity_spatial_preview_mirror_get_type ())

/**
 * UnitySpatialPreviewMirror:
 *
 * A live window in the widget's area. The compositor draws the window into a
 * subsurface below the widget's surface, and the widget clears its own area of
 * that surface, so the window shows through and everything GTK draws after the
 * widget shows above it.
 */
G_DECLARE_FINAL_TYPE (UnitySpatialPreviewMirror, unity_spatial_preview_mirror, UNITY_SPATIAL, PREVIEW_MIRROR, GtkWidget)

/**
 * unity_spatial_preview_mirror_new:
 * @view_id: the wayfire view id of the window
 *
 * Returns: (transfer full): a new #UnitySpatialPreviewMirror
 */
GtkWidget *unity_spatial_preview_mirror_new         (guint view_id);

/**
 * unity_spatial_preview_mirror_get_view_id:
 * @self: a #UnitySpatialPreviewMirror
 *
 * Returns: the wayfire view id of the window
 */
guint      unity_spatial_preview_mirror_get_view_id (UnitySpatialPreviewMirror *self);

/**
 * unity_spatial_preview_mirror_set_view_id:
 * @self: a #UnitySpatialPreviewMirror
 * @view_id: the wayfire view id of the window
 *
 * Shows another window.
 */
void       unity_spatial_preview_mirror_set_view_id (UnitySpatialPreviewMirror *self,
                                                     guint                      view_id);

G_END_DECLS
