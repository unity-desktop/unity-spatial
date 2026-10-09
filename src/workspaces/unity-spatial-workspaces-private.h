/* unity-spatial-workspaces-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "unity-spatial-workspace-card-private.h"

G_BEGIN_DECLS

GskTransform              *unity_spatial_zoom_onto (const graphene_rect_t *from,
                                                    const graphene_rect_t *to);

#define UNITY_SPATIAL_TYPE_WORKSPACES (unity_spatial_workspaces_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWorkspaces, unity_spatial_workspaces, UNITY_SPATIAL, WORKSPACES, GtkGrid)

UnitySpatialWorkspaceCard *unity_spatial_workspaces_get_current_card (UnitySpatialWorkspaces *self);

void                       unity_spatial_workspaces_set_progress     (UnitySpatialWorkspaces *self,
                                                                      gdouble                 progress);

void                       unity_spatial_workspaces_set_area         (UnitySpatialWorkspaces *self,
                                                                      gint                    width,
                                                                      gint                    height);

const GdkRectangle        *unity_spatial_workspaces_get_slot         (UnitySpatialWorkspaces *self);

graphene_rect_t            unity_spatial_workspaces_get_card_rect    (UnitySpatialWorkspaces *self,
                                                                      gdouble                 x,
                                                                      gdouble                 y);

GskTransform              *unity_spatial_workspaces_get_zoom         (UnitySpatialWorkspaces *self,
                                                                      gdouble                 zoom);

G_END_DECLS
