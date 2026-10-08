/* unity-spatial-gesture-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_GESTURE (unity_spatial_gesture_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialGesture, unity_spatial_gesture, UNITY_SPATIAL, GESTURE, GObject)

UnitySpatialGesture *unity_spatial_gesture_new_swipe (guint fingers);

UnitySpatialGesture *unity_spatial_gesture_new_pinch (guint fingers);

G_END_DECLS
