/* unity-spatial-view-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-carousel-private.h"
#include "unity-spatial-view.h"

G_BEGIN_DECLS

UnitySpatialCarousel *unity_spatial_view_get_carousel (UnitySpatialView *self);

void                  unity_spatial_view_begin_swipe  (UnitySpatialView *self);

void                  unity_spatial_view_update_swipe (UnitySpatialView *self,
                                                       gdouble           progress);

void                  unity_spatial_view_end_swipe    (UnitySpatialView *self,
                                                       gdouble           velocity,
                                                       gdouble           to);

G_END_DECLS
