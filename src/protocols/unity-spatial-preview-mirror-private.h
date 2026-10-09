/* unity-spatial-preview-mirror-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wayland-client.h>

#include "unity-spatial-preview-mirror.h"

G_BEGIN_DECLS

GdkRectangle unity_spatial_preview_mirror_snap_rect    (const graphene_rect_t     *rect);

gboolean     unity_spatial_preview_mirror_sync         (UnitySpatialPreviewMirror *self);

void         unity_spatial_preview_mirror_place_above  (UnitySpatialPreviewMirror *self,
                                                        struct wl_surface         *parent);

G_END_DECLS
