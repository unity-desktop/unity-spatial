/* unity-spatial-overview-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-stage.h"
#include "unity-spatial-workspaces-private.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_OVERVIEW (unity_spatial_overview_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialOverview, unity_spatial_overview, UNITY_SPATIAL, OVERVIEW, GObject)

UnitySpatialOverview *unity_spatial_overview_new          (UnitySpatialStage         *stage,
                                                           UnitySpatialWorkspaces    *workspaces);

GskTransform         *unity_spatial_overview_get_zoom     (UnitySpatialOverview      *self,
                                                           gdouble                    morph);

gboolean              unity_spatial_overview_slide_active (UnitySpatialOverview      *self);

void                  unity_spatial_overview_slide_begin  (UnitySpatialOverview      *self);

void                  unity_spatial_overview_slide_update (UnitySpatialOverview      *self,
                                                           guint32                    time,
                                                           gdouble                    dx,
                                                           gdouble                    dy);

void                  unity_spatial_overview_slide_end    (UnitySpatialOverview      *self,
                                                           gboolean                   cancelled);

void                  unity_spatial_overview_slide_reset  (UnitySpatialOverview      *self);

G_END_DECLS
