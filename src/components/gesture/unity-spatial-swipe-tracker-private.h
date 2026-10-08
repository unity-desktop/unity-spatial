/* unity-spatial-swipe-tracker-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef struct _UnitySpatialSwipeTracker UnitySpatialSwipeTracker;

UnitySpatialSwipeTracker *unity_spatial_swipe_tracker_new    (gdouble                   from,
                                                              gdouble                   lower,
                                                              gdouble                   upper);
void                      unity_spatial_swipe_tracker_free   (UnitySpatialSwipeTracker *self);
gdouble                   unity_spatial_swipe_tracker_update (UnitySpatialSwipeTracker *self,
                                                              guint32                   time,
                                                              gdouble                   delta);
gdouble                   unity_spatial_swipe_tracker_end    (UnitySpatialSwipeTracker *self,
                                                              gboolean                  cancelled);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (UnitySpatialSwipeTracker, unity_spatial_swipe_tracker_free)

G_END_DECLS
