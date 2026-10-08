/* unity-spatial-workspace-view-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-workspace-view.h"

G_BEGIN_DECLS

typedef struct
{
  const gchar *name;
  gint         width;
  gint         height;
  GdkRectangle workarea;
  guint        desktop_view_id;
  gint         grid_width;
  gint         grid_height;
  gint         x;
  gint         y;
} UnitySpatialOutputState;

void unity_spatial_workspace_view_update (UnitySpatialWorkspaceView     *self,
                                          const UnitySpatialOutputState *output);

G_END_DECLS
