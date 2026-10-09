/* unity-spatial-workspace-card-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "unity-spatial-workspace-page.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_WORKSPACE_CARD (unity_spatial_workspace_card_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWorkspaceCard, unity_spatial_workspace_card, UNITY_SPATIAL, WORKSPACE_CARD, GtkWidget)

GtkWidget                 *unity_spatial_workspace_card_new           (UnitySpatialWorkspacePage *workspace);

void                       unity_spatial_workspace_card_set_morph     (UnitySpatialWorkspaceCard *self,
                                                                       gdouble                    morph);

void                       unity_spatial_workspace_card_set_wall      (UnitySpatialWorkspaceCard *self,
                                                                       gboolean                   wall);

G_END_DECLS
