/* unity-spatial-carousel-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

#include "unity-spatial-workspace-thumbnail-private.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_CAROUSEL (unity_spatial_carousel_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialCarousel, unity_spatial_carousel, UNITY_SPATIAL, CAROUSEL, GtkWidget)

UnitySpatialWorkspaceThumbnail *unity_spatial_carousel_get_current (UnitySpatialCarousel *self);

void                       unity_spatial_carousel_set_progress     (UnitySpatialCarousel *self,
                                                                      gdouble                 progress);

gdouble                    unity_spatial_carousel_get_position     (UnitySpatialCarousel *self,
                                                                      GtkOrientation          axis);

void                       unity_spatial_carousel_begin_swipe      (UnitySpatialCarousel *self,
                                                                      GtkOrientation          axis);

void                       unity_spatial_carousel_update_swipe     (UnitySpatialCarousel *self,
                                                                      gdouble                 progress);

void                       unity_spatial_carousel_end_swipe        (UnitySpatialCarousel *self,
                                                                      gdouble                 velocity,
                                                                      gdouble                 to);

G_END_DECLS
