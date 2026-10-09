/* unity-spatial-window-page.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-window-page-private.h"

struct _UnitySpatialWindowPage
{
  GObject parent_instance;

  guint        view_id;
  gchar       *app_id;
  gchar       *title;
  GdkRectangle bounds;
  gint         workspace_x;
  gint         workspace_y;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWindowPage, unity_spatial_window_page, G_TYPE_OBJECT)

typedef enum
{
  PROP_VIEW_ID = 1,
  PROP_APP_ID,
  PROP_TITLE,
  PROP_BOUNDS,
  PROP_WORKSPACE_X,
  PROP_WORKSPACE_Y,
} UnitySpatialWindowPageProperty;

static GParamSpec *properties[PROP_WORKSPACE_Y + 1];

static void
unity_spatial_window_page_finalize (GObject *object)
{
  UnitySpatialWindowPage *self = UNITY_SPATIAL_WINDOW_PAGE (object);

  g_free (self->app_id);
  g_free (self->title);

  G_OBJECT_CLASS (unity_spatial_window_page_parent_class)->finalize (object);
}

static void
unity_spatial_window_page_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  UnitySpatialWindowPage *self = UNITY_SPATIAL_WINDOW_PAGE (object);

  switch ((UnitySpatialWindowPageProperty) prop_id)
    {
    case PROP_VIEW_ID:
      g_value_set_uint (value, self->view_id);
      break;
    case PROP_APP_ID:
      g_value_set_string (value, self->app_id);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_BOUNDS:
      g_value_set_boxed (value, &self->bounds);
      break;
    case PROP_WORKSPACE_X:
      g_value_set_int (value, self->workspace_x);
      break;
    case PROP_WORKSPACE_Y:
      g_value_set_int (value, self->workspace_y);
      break;
    }
}

static void
unity_spatial_window_page_class_init (UnitySpatialWindowPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = unity_spatial_window_page_finalize;
  object_class->get_property = unity_spatial_window_page_get_property;

  properties[PROP_VIEW_ID] =
    g_param_spec_uint ("view-id", NULL, NULL, 0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_APP_ID] =
    g_param_spec_string ("app-id", NULL, NULL, NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL, NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_BOUNDS] =
    g_param_spec_boxed ("bounds", NULL, NULL, GDK_TYPE_RECTANGLE,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_WORKSPACE_X] =
    g_param_spec_int ("workspace-x", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_WORKSPACE_Y] =
    g_param_spec_int ("workspace-y", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_spatial_window_page_init (UnitySpatialWindowPage *self)
{
}

UnitySpatialWindowPage *
unity_spatial_window_page_new (guint view_id)
{
  UnitySpatialWindowPage *self = g_object_new (UNITY_SPATIAL_TYPE_WINDOW_PAGE, NULL);

  self->view_id = view_id;

  return self;
}

static void
set_string (UnitySpatialWindowPage          *self,
            gchar                          **field,
            const gchar                     *value,
            UnitySpatialWindowPageProperty   prop)
{
  if (g_set_str (field, value))
    g_object_notify_by_pspec (G_OBJECT (self), properties[prop]);
}

static void
set_int (UnitySpatialWindowPage         *self,
         gint                           *field,
         gint                            value,
         UnitySpatialWindowPageProperty  prop)
{
  if (*field == value)
    return;

  *field = value;
  g_object_notify_by_pspec (G_OBJECT (self), properties[prop]);
}

void
unity_spatial_window_page_update (UnitySpatialWindowPage        *self,
                                  const UnitySpatialWindowState *state)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self));

  g_object_freeze_notify (G_OBJECT (self));

  set_string (self, &self->app_id, state->app_id, PROP_APP_ID);
  set_string (self, &self->title, state->title, PROP_TITLE);
  unity_spatial_window_page_set_workspace (self, state->workspace_x, state->workspace_y);

  if (!gdk_rectangle_equal (&self->bounds, &state->bounds))
    {
      self->bounds = state->bounds;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BOUNDS]);
    }

  g_object_thaw_notify (G_OBJECT (self));
}

void
unity_spatial_window_page_set_workspace (UnitySpatialWindowPage *self,
                                         gint                    workspace_x,
                                         gint                    workspace_y)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self));

  g_object_freeze_notify (G_OBJECT (self));
  set_int (self, &self->workspace_x, workspace_x, PROP_WORKSPACE_X);
  set_int (self, &self->workspace_y, workspace_y, PROP_WORKSPACE_Y);
  g_object_thaw_notify (G_OBJECT (self));
}

guint
unity_spatial_window_page_get_view_id (UnitySpatialWindowPage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self), 0);

  return self->view_id;
}

const gchar *
unity_spatial_window_page_get_app_id (UnitySpatialWindowPage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self), NULL);

  return self->app_id;
}

const gchar *
unity_spatial_window_page_get_title (UnitySpatialWindowPage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self), NULL);

  return self->title;
}

const GdkRectangle *
unity_spatial_window_page_get_bounds (UnitySpatialWindowPage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self), NULL);

  return &self->bounds;
}

gint
unity_spatial_window_page_get_workspace_x (UnitySpatialWindowPage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self), 0);

  return self->workspace_x;
}

gint
unity_spatial_window_page_get_workspace_y (UnitySpatialWindowPage *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (self), 0);

  return self->workspace_y;
}
