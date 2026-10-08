/* unity-spatial-workspace-view.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-workspace-view-private.h"

#include "unity-spatial-wayfire-private.h"
#include "unity-spatial-workspace-page-private.h"

struct _UnitySpatialWorkspaceView
{
  GObject parent_instance;

  GPtrArray                 *workspaces;
  UnitySpatialWorkspacePage *current;
  gchar                     *output_name;
  gint                       output_width;
  gint                       output_height;
  GdkRectangle               workarea;
  guint                      desktop_view_id;
  gint                       grid_width;
  gint                       grid_height;
};

static void unity_spatial_workspace_view_list_model_init (GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (UnitySpatialWorkspaceView, unity_spatial_workspace_view, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, unity_spatial_workspace_view_list_model_init))

typedef enum
{
  PROP_CURRENT = 1,
  PROP_GRID_WIDTH,
  PROP_GRID_HEIGHT,
  PROP_OUTPUT_NAME,
  PROP_WORKAREA,
  PROP_DESKTOP_VIEW_ID,
} UnitySpatialWorkspaceViewProperty;

static GParamSpec *properties[PROP_DESKTOP_VIEW_ID + 1];

static GType
unity_spatial_workspace_view_get_item_type (GListModel *model)
{
  return UNITY_SPATIAL_TYPE_WORKSPACE_PAGE;
}

static guint
unity_spatial_workspace_view_get_n_items (GListModel *model)
{
  return UNITY_SPATIAL_WORKSPACE_VIEW (model)->workspaces->len;
}

static gpointer
unity_spatial_workspace_view_get_item (GListModel *model,
                                       guint       position)
{
  UnitySpatialWorkspaceView *self = UNITY_SPATIAL_WORKSPACE_VIEW (model);

  if (position >= self->workspaces->len)
    return NULL;

  return g_object_ref (g_ptr_array_index (self->workspaces, position));
}

static void
unity_spatial_workspace_view_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = unity_spatial_workspace_view_get_item_type;
  iface->get_n_items   = unity_spatial_workspace_view_get_n_items;
  iface->get_item      = unity_spatial_workspace_view_get_item;
}

static void
set_grid (UnitySpatialWorkspaceView *self,
          gint                       width,
          gint                       height)
{
  guint removed = self->workspaces->len;

  if (self->grid_width == width && self->grid_height == height)
    return;

  self->current     = NULL;
  self->grid_width  = width;
  self->grid_height = height;
  g_ptr_array_set_size (self->workspaces, 0);

  for (gint y = 0; y < height; y++)
    for (gint x = 0; x < width; x++)
      g_ptr_array_add (self->workspaces, unity_spatial_workspace_page_new (x, y));

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, self->workspaces->len);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_GRID_WIDTH]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_GRID_HEIGHT]);
}

static void
set_current (UnitySpatialWorkspaceView *self,
             gint                       x,
             gint                       y)
{
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_workspace (self, x, y);

  if (self->current == current)
    return;

  self->current = current;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CURRENT]);
}

static void
unity_spatial_workspace_view_dispose (GObject *object)
{
  UnitySpatialWorkspaceView *self = UNITY_SPATIAL_WORKSPACE_VIEW (object);

  self->current = NULL;
  g_clear_pointer (&self->workspaces, g_ptr_array_unref);
  g_clear_pointer (&self->output_name, g_free);

  G_OBJECT_CLASS (unity_spatial_workspace_view_parent_class)->dispose (object);
}

static void
unity_spatial_workspace_view_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  UnitySpatialWorkspaceView *self = UNITY_SPATIAL_WORKSPACE_VIEW (object);

  switch ((UnitySpatialWorkspaceViewProperty) prop_id)
    {
    case PROP_CURRENT:
      g_value_set_object (value, self->current);
      break;
    case PROP_GRID_WIDTH:
      g_value_set_int (value, self->grid_width);
      break;
    case PROP_GRID_HEIGHT:
      g_value_set_int (value, self->grid_height);
      break;
    case PROP_OUTPUT_NAME:
      g_value_set_string (value, self->output_name);
      break;
    case PROP_WORKAREA:
      g_value_set_boxed (value, &self->workarea);
      break;
    case PROP_DESKTOP_VIEW_ID:
      g_value_set_uint (value, self->desktop_view_id);
      break;
    }
}

static void
unity_spatial_workspace_view_class_init (UnitySpatialWorkspaceViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = unity_spatial_workspace_view_dispose;
  object_class->get_property = unity_spatial_workspace_view_get_property;

  properties[PROP_CURRENT] =
    g_param_spec_object ("current", NULL, NULL, UNITY_SPATIAL_TYPE_WORKSPACE_PAGE,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_GRID_WIDTH] =
    g_param_spec_int ("grid-width", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_GRID_HEIGHT] =
    g_param_spec_int ("grid-height", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_OUTPUT_NAME] =
    g_param_spec_string ("output-name", NULL, NULL, NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_WORKAREA] =
    g_param_spec_boxed ("workarea", NULL, NULL, GDK_TYPE_RECTANGLE,
                        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_DESKTOP_VIEW_ID] =
    g_param_spec_uint ("desktop-view-id", NULL, NULL, 0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_spatial_workspace_view_init (UnitySpatialWorkspaceView *self)
{
  self->workspaces = g_ptr_array_new_with_free_func (g_object_unref);
}

void
unity_spatial_workspace_view_update (UnitySpatialWorkspaceView     *self,
                                     const UnitySpatialOutputState *output)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self));

  g_object_freeze_notify (G_OBJECT (self));

  self->output_width  = output->width;
  self->output_height = output->height;

  if (g_set_str (&self->output_name, output->name))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_OUTPUT_NAME]);

  if (!gdk_rectangle_equal (&self->workarea, &output->workarea))
    {
      self->workarea = output->workarea;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WORKAREA]);
    }

  if (self->desktop_view_id != output->desktop_view_id)
    {
      self->desktop_view_id = output->desktop_view_id;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DESKTOP_VIEW_ID]);
    }

  set_grid (self, output->grid_width, output->grid_height);
  set_current (self, output->x, output->y);

  g_object_thaw_notify (G_OBJECT (self));
}

UnitySpatialWorkspaceView *
unity_spatial_workspace_view_get_default (void)
{
  return unity_spatial_wayfire_get_workspace_view (unity_spatial_wayfire_get_default ());
}

UnitySpatialWorkspacePage *
unity_spatial_workspace_view_get_workspace (UnitySpatialWorkspaceView *self,
                                            gint                       x,
                                            gint                       y)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), NULL);

  if (x < 0 || x >= self->grid_width || y < 0 || y >= self->grid_height)
    return NULL;

  return g_ptr_array_index (self->workspaces, y * self->grid_width + x);
}

UnitySpatialWorkspacePage *
unity_spatial_workspace_view_get_current (UnitySpatialWorkspaceView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), NULL);

  return self->current;
}

gint
unity_spatial_workspace_view_get_grid_width (UnitySpatialWorkspaceView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), 0);

  return self->grid_width;
}

gint
unity_spatial_workspace_view_get_grid_height (UnitySpatialWorkspaceView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), 0);

  return self->grid_height;
}

const gchar *
unity_spatial_workspace_view_get_output_name (UnitySpatialWorkspaceView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), NULL);

  return self->output_name;
}

void
unity_spatial_workspace_view_get_output_size (UnitySpatialWorkspaceView *self,
                                              gint                      *width,
                                              gint                      *height)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self));

  *width  = self->output_width;
  *height = self->output_height;
}

const GdkRectangle *
unity_spatial_workspace_view_get_workarea (UnitySpatialWorkspaceView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), NULL);

  return &self->workarea;
}

guint
unity_spatial_workspace_view_get_desktop_view_id (UnitySpatialWorkspaceView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self), 0);

  return self->desktop_view_id;
}

void
unity_spatial_workspace_view_activate (UnitySpatialWorkspaceView *self,
                                       UnitySpatialWorkspacePage *workspace)
{
  gint x;
  gint y;

  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACE_VIEW (self));
  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACE_PAGE (workspace));

  x = unity_spatial_workspace_page_get_x (workspace);
  y = unity_spatial_workspace_page_get_y (workspace);
  unity_spatial_wayfire_set_workspace (unity_spatial_wayfire_get_default (), x, y);
  set_current (self, x, y);
}
