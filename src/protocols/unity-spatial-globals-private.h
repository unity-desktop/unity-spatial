/* unity-spatial-globals-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>
#include <wayland-client.h>

#include "single-pixel-buffer-v1-client-protocol.h"
#include "unity-spatial-gestures-unstable-client-protocol.h"
#include "unity-spatial-preview-unstable-client-protocol.h"
#include "viewporter-client-protocol.h"

G_BEGIN_DECLS

typedef struct
{
  struct wl_compositor                     *compositor;
  struct wl_subcompositor                  *subcompositor;
  struct wp_viewporter                     *viewporter;
  struct wp_single_pixel_buffer_manager_v1 *single_pixel;
  struct zunity_preview_manager            *previews;
  struct zunity_spatial_gestures           *gestures;
  struct wl_buffer                         *clear_buffer;
  struct wl_region                         *empty_region;
} UnitySpatialGlobals;

const UnitySpatialGlobals *unity_spatial_globals_get (void);

G_END_DECLS
