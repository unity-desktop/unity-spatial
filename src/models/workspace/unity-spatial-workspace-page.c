/* unity-spatial-workspace-page.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-workspace-page-private.h"

struct _UnitySpatialWorkspacePage
{
  GObject parent_instance;

  gint x;
  gint y;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWorkspacePage, unity_spatial_workspace_page, G_TYPE_OBJECT)

typedef enum
{
  PROP_X = 1,
  PROP_Y,
} UnitySpatialWorkspacePageProperty;

static GParamSpec *properties[PROP_Y + 1];

static void
unity_spatial_workspace_page_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  UnitySpatialWorkspacePage *self = UNITY_SPATIAL_WORKSPACE_PAGE (object);

  switch ((UnitySpatialWorkspacePageProperty) prop_id)
    {
    case PROP_X:
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;
    }
}

static void
unity_spatial_workspace_page_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  UnitySpatialWorkspacePage *self = UNITY_SPATIAL_WORKSPACE_PAGE (object);

  switch ((UnitySpatialWorkspacePageProperty) prop_id)
    {
    case PROP_X:
      self->x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      break;
    }
}

static void
unity_spatial_workspace_page_class_init (UnitySpatialWorkspacePageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = unity_spatial_workspace_page_get_property;
  object_class->set_property = unity_spatial_workspace_page_set_property;

  properties[PROP_X] =
    g_param_spec_int ("x", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  properties[PROP_Y] =
    g_param_spec_int ("y", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_spatial_workspace_page_init (UnitySpatialWorkspacePage *self)
{
}

UnitySpatialWorkspacePage *
unity_spatial_workspace_page_new (gint x,
                                  gint y)
{
  return g_object_new (UNITY_SPATIAL_TYPE_WORKSPACE_PAGE, "x", x, "y", y, NULL);
}

gint
unity_spatial_workspace_page_get_x (UnitySpatialWorkspacePage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_PAGE (self), 0);

  return self->x;
}

gint
unity_spatial_workspace_page_get_y (UnitySpatialWorkspacePage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_PAGE (self), 0);

  return self->y;
}
