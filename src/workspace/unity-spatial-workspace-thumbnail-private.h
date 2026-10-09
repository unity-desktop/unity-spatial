/* unity-spatial-workspace-thumbnail-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "unity-spatial-workspace-page.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WORKSPACE_THUMBNAIL (unity_spatial_workspace_thumbnail_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWorkspaceThumbnail, unity_spatial_workspace_thumbnail, UNITY_SPATIAL, WORKSPACE_THUMBNAIL, GtkWidget)

GtkWidget                 *unity_spatial_workspace_thumbnail_new           (UnitySpatialWorkspacePage *workspace);

UnitySpatialWorkspacePage *unity_spatial_workspace_thumbnail_get_workspace (UnitySpatialWorkspaceThumbnail *self);

void                       unity_spatial_workspace_thumbnail_set_morph     (UnitySpatialWorkspaceThumbnail *self,
                                                                       gdouble                    morph);

void                       unity_spatial_workspace_thumbnail_set_wall      (UnitySpatialWorkspaceThumbnail *self,
                                                                       gboolean                   wall);

G_END_DECLS
