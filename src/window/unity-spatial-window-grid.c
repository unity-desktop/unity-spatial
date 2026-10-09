/* unity-spatial-window-grid.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-window-grid-private.h"

#include <math.h>

#include "unity-spatial-mirror-stack-private.h"
#include "unity-spatial-preview-mirror.h"
#include "unity-spatial-window-layout-private.h"
#include "unity-spatial-window-page.h"
#include "unity-spatial-window-thumbnail-private.h"

struct _UnitySpatialWindowGrid
{
  GtkWidget parent_instance;

  GListModel                  *model;
  GPtrArray                   *thumbnails;
  gdouble                      morph;
  gboolean                     wall;
  gboolean                     chrome;
  UnitySpatialWindowThumbnail *dragged;
  GtkEventController          *drag_motion;
  gdouble                      drag_x;
  gdouble                      drag_y;
  gdouble                      pointer_x;
  gdouble                      pointer_y;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWindowGrid, unity_spatial_window_grid, GTK_TYPE_WIDGET)

typedef enum
{
  PROP_MORPH = 1,
  PROP_WALL,
  PROP_CHROME,
  PROP_SPACING,
} UnitySpatialWindowGridProperty;

static GParamSpec *properties[PROP_SPACING + 1];

static UnitySpatialWindowLayout *
layout_of (UnitySpatialWindowGrid *self)
{
  return UNITY_SPATIAL_WINDOW_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));
}

static void
update_chrome (UnitySpatialWindowGrid *self)
{
  gboolean chrome = self->morph >= 1 && !self->wall;

  if (self->chrome == chrome)
    return;

  self->chrome = chrome;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CHROME]);
}

static void
items_changed_cb (UnitySpatialWindowGrid *self,
                  guint                   position,
                  guint                   removed,
                  guint                   added)
{
  g_autoptr (GHashTable) kept = g_hash_table_new (NULL, NULL);
  GHashTableIter         iter;
  gpointer               thumbnail;

  for (guint i = 0; i < removed; i++)
    {
      thumbnail = g_ptr_array_steal_index (self->thumbnails, position);
      g_hash_table_insert (kept, unity_spatial_window_thumbnail_get_page (thumbnail), thumbnail);
    }

  for (guint i = 0; i < added; i++)
    {
      g_autoptr (UnitySpatialWindowPage) page  = g_list_model_get_item (self->model, position + i);
      GtkWidget                         *below = position + i < self->thumbnails->len
                                                 ? g_ptr_array_index (self->thumbnails, position + i)
                                                 : NULL;

      if (!g_hash_table_steal_extended (kept, page, NULL, &thumbnail))
        {
          thumbnail = unity_spatial_window_thumbnail_new (page);
          g_object_bind_property (self, "chrome", thumbnail, "chrome", G_BINDING_SYNC_CREATE);
        }

      gtk_widget_insert_after (thumbnail, GTK_WIDGET (self), below);
      g_ptr_array_insert (self->thumbnails, position + i, thumbnail);
    }

  g_hash_table_iter_init (&iter, kept);
  while (g_hash_table_iter_next (&iter, NULL, &thumbnail))
    gtk_widget_unparent (thumbnail);

  unity_spatial_mirror_stack_invalidate (gtk_widget_get_native (GTK_WIDGET (self)));
}

static GdkContentProvider *
drag_prepare_cb (UnitySpatialWindowGrid *self,
                 gdouble                 x,
                 gdouble                 y)
{
  GtkWidget *picked    = self->morph >= 1 ? gtk_widget_pick (GTK_WIDGET (self), x, y, GTK_PICK_DEFAULT) : NULL;
  GtkWidget *thumbnail = picked != NULL ? gtk_widget_get_ancestor (picked, UNITY_SPATIAL_TYPE_WINDOW_THUMBNAIL) : NULL;

  if (thumbnail == NULL)
    return NULL;

  g_set_object (&self->dragged, UNITY_SPATIAL_WINDOW_THUMBNAIL (thumbnail));
  self->drag_x = x;
  self->drag_y = y;

  return gdk_content_provider_new_typed (UNITY_SPATIAL_TYPE_WINDOW_PAGE,
                                         unity_spatial_window_thumbnail_get_page (UNITY_SPATIAL_WINDOW_THUMBNAIL (thumbnail)));
}

static void
drag_motion_cb (UnitySpatialWindowGrid *self,
                gdouble                 x,
                gdouble                 y)
{
  self->pointer_x = x;
  self->pointer_y = y;
}

static void
drag_begin_cb (UnitySpatialWindowGrid *self,
               GdkDrag                *drag)
{
  UnitySpatialWindowThumbnail *thumbnail = self->dragged;
  GtkWidget                   *native    = GTK_WIDGET (gtk_widget_get_native (GTK_WIDGET (self)));
  GtkWidget                   *mirror;
  graphene_rect_t              bounds;
  graphene_point_t             pointer;

  if (!gtk_widget_compute_bounds (GTK_WIDGET (thumbnail), native, &bounds) ||
      !gtk_widget_compute_point (GTK_WIDGET (self), native, &GRAPHENE_POINT_INIT (self->drag_x, self->drag_y),
                                 &pointer))
    return;

  mirror = unity_spatial_preview_mirror_new (
    unity_spatial_window_page_get_view_id (unity_spatial_window_thumbnail_get_page (thumbnail)));
  gtk_widget_set_size_request (mirror, ceil (bounds.size.width), ceil (bounds.size.height));
  gtk_drag_icon_set_child (GTK_DRAG_ICON (gtk_drag_icon_get_for_drag (drag)), mirror);
  gdk_drag_set_hotspot (drag, pointer.x - bounds.origin.x, pointer.y - bounds.origin.y);
  gtk_widget_set_child_visible (GTK_WIDGET (self->dragged), FALSE);

  self->pointer_x   = pointer.x;
  self->pointer_y   = pointer.y;
  self->drag_motion = gtk_drop_controller_motion_new ();
  g_signal_connect_object (self->drag_motion, "motion", G_CALLBACK (drag_motion_cb), self, G_CONNECT_SWAPPED);
  gtk_widget_add_controller (native, self->drag_motion);
}

static void
drag_end_cb (UnitySpatialWindowGrid *self,
             GdkDrag                *drag,
             gboolean                moved)
{
  GtkWidget       *native = GTK_WIDGET (gtk_widget_get_native (GTK_WIDGET (self)));
  graphene_point_t end;

  if (self->drag_motion != NULL && native != NULL)
    gtk_widget_remove_controller (native, self->drag_motion);
  self->drag_motion = NULL;

  if (self->dragged != NULL && !moved && gtk_widget_get_parent (GTK_WIDGET (self->dragged)) == GTK_WIDGET (self))
    {
      if (gtk_widget_compute_point (native, GTK_WIDGET (self), &GRAPHENE_POINT_INIT (self->pointer_x, self->pointer_y),
                                    &end))
        unity_spatial_window_layout_fly_from (layout_of (self), GTK_WIDGET (self->dragged), end.x - self->drag_x,
                                              end.y - self->drag_y);
      gtk_widget_set_child_visible (GTK_WIDGET (self->dragged), TRUE);
    }
  g_clear_object (&self->dragged);
}

static void
unity_spatial_window_grid_dispose (GObject *object)
{
  UnitySpatialWindowGrid *self = UNITY_SPATIAL_WINDOW_GRID (object);
  GtkWidget              *child;

  if (self->drag_motion != NULL)
    gtk_widget_remove_controller (gtk_event_controller_get_widget (self->drag_motion), g_steal_pointer (&self->drag_motion));
  g_clear_object (&self->model);
  g_clear_object (&self->dragged);
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))) != NULL)
    gtk_widget_unparent (child);
  g_clear_pointer (&self->thumbnails, g_ptr_array_unref);

  G_OBJECT_CLASS (unity_spatial_window_grid_parent_class)->dispose (object);
}

static void
unity_spatial_window_grid_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  UnitySpatialWindowGrid *self = UNITY_SPATIAL_WINDOW_GRID (object);

  switch ((UnitySpatialWindowGridProperty) prop_id)
    {
    case PROP_MORPH:
      g_value_set_double (value, self->morph);
      break;
    case PROP_WALL:
      g_value_set_boolean (value, self->wall);
      break;
    case PROP_CHROME:
      g_value_set_boolean (value, self->chrome);
      break;
    case PROP_SPACING:
      g_object_get_property (G_OBJECT (layout_of (self)), "spacing", value);
      break;
    }
}

static void
unity_spatial_window_grid_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  UnitySpatialWindowGrid *self = UNITY_SPATIAL_WINDOW_GRID (object);

  switch ((UnitySpatialWindowGridProperty) prop_id)
    {
    case PROP_MORPH:
      if (G_APPROX_VALUE (self->morph, g_value_get_double (value), DBL_EPSILON))
        break;
      self->morph = g_value_get_double (value);
      update_chrome (self);
      g_object_notify_by_pspec (object, pspec);
      break;
    case PROP_WALL:
      if (self->wall == g_value_get_boolean (value))
        break;
      self->wall = g_value_get_boolean (value);
      gtk_widget_set_can_focus (GTK_WIDGET (self), !self->wall);
      update_chrome (self);
      g_object_notify_by_pspec (object, pspec);
      break;
    case PROP_SPACING:
      g_object_set_property (G_OBJECT (layout_of (self)), "spacing", value);
      break;
    case PROP_CHROME:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
unity_spatial_window_grid_class_init (UnitySpatialWindowGridClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = unity_spatial_window_grid_dispose;
  object_class->get_property = unity_spatial_window_grid_get_property;
  object_class->set_property = unity_spatial_window_grid_set_property;

  /**
   * UnitySpatialWindowGrid:morph:
   *
   * The position between the windows where they are on the desktop (0) and
   * their places in the grid (1).
   */
  properties[PROP_MORPH] =
    g_param_spec_double ("morph", NULL, NULL, 0, 1, 1,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWindowGrid:wall:
   *
   * Whether the grid shows on the workspaces page, where its windows are
   * small and show no chrome.
   */
  properties[PROP_WALL] =
    g_param_spec_boolean ("wall", NULL, NULL, FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWindowGrid:chrome:
   *
   * Whether the windows show their chrome: on the windows page only.
   */
  properties[PROP_CHROME] =
    g_param_spec_boolean ("chrome", NULL, NULL, TRUE, G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWindowGrid:spacing:
   *
   * The gap between two windows, in pixels.
   */
  properties[PROP_SPACING] =
    g_param_spec_int ("spacing", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_layout_manager_type (widget_class, UNITY_SPATIAL_TYPE_WINDOW_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "windowgrid");
}

static void
unity_spatial_window_grid_init (UnitySpatialWindowGrid *self)
{
  GtkDragSource *drag = gtk_drag_source_new ();

  self->morph      = 1;
  self->chrome     = TRUE;
  self->thumbnails = g_ptr_array_new ();

  g_object_bind_property (self, "morph", layout_of (self), "morph", G_BINDING_SYNC_CREATE);

  gtk_drag_source_set_actions (drag, GDK_ACTION_MOVE);
  g_signal_connect_swapped (drag, "prepare", G_CALLBACK (drag_prepare_cb), self);
  g_signal_connect_swapped (drag, "drag-begin", G_CALLBACK (drag_begin_cb), self);
  g_signal_connect_swapped (drag, "drag-end", G_CALLBACK (drag_end_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag));
}

void
unity_spatial_window_grid_set_model (UnitySpatialWindowGrid *self,
                                     GListModel             *model)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_GRID (self));
  g_return_if_fail (G_IS_LIST_MODEL (model));
  g_return_if_fail (self->model == NULL);

  self->model = g_object_ref (model);
  g_signal_connect_object (model, "items-changed", G_CALLBACK (items_changed_cb), self, G_CONNECT_SWAPPED);
  items_changed_cb (self, 0, 0, g_list_model_get_n_items (model));
}
