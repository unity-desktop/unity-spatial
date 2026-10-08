/* unity-spatial-window-page-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-window-page.h"

G_BEGIN_DECLS

typedef struct
{
  guint        view_id;
  const gchar *app_id;
  const gchar *title;
  GdkRectangle bounds;
  gint         workspace_x;
  gint         workspace_y;
  gboolean     minimized;
} UnitySpatialWindowState;

UnitySpatialWindowPage *unity_spatial_window_page_new           (guint                          view_id);

void                    unity_spatial_window_page_update        (UnitySpatialWindowPage        *self,
                                                                 const UnitySpatialWindowState *state);

void                    unity_spatial_window_page_set_workspace (UnitySpatialWindowPage        *self,
                                                                 gint                           workspace_x,
                                                                 gint                           workspace_y);

G_END_DECLS
