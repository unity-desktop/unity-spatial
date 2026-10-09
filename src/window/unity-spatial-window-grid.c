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

static UnitySpatialWindowLayout *
layout_of (UnitySpatialWindowGrid *self)
{
  return UNITY_SPATIAL_WINDOW_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));
}

static void
apply_chrome (UnitySpatialWindowGrid *self)
{
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    unity_spatial_window_thumbnail_set_chrome_visible (UNITY_SPATIAL_WINDOW_THUMBNAIL (child), self->chrome);
}

static void
update_chrome (UnitySpatialWindowGrid *self)
{
  gboolean chrome = self->morph >= 1 && !self->wall;

  if (self->chrome == chrome)
    return;

  self->chrome = chrome;
  apply_chrome (self);
}

static void
items_changed_cb (UnitySpatialWindowGrid *self,
                  guint                   position,
                  guint                   removed,
                  guint                   added)
{
  for (guint i = 0; i < removed; i++)
    gtk_widget_unparent (g_ptr_array_steal_index (self->thumbnails, position));

  for (guint i = 0; i < added; i++)
    {
      g_autoptr (UnitySpatialWindowPage) page = g_list_model_get_item (self->model, position + i);
      UnitySpatialWindowThumbnail       *thumbnail = unity_spatial_window_thumbnail_new (page);
      GtkWidget                         *below     = position + i < self->thumbnails->len
                                                     ? g_ptr_array_index (self->thumbnails, position + i)
                                                     : NULL;

      gtk_widget_insert_after (GTK_WIDGET (thumbnail), GTK_WIDGET (self), below);
      unity_spatial_window_thumbnail_set_chrome_visible (thumbnail, self->chrome);
      g_ptr_array_insert (self->thumbnails, position + i, thumbnail);
    }

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

  if (self->dragged != NULL && !moved)
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
unity_spatial_window_grid_class_init (UnitySpatialWindowGridClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_spatial_window_grid_dispose;

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

void
unity_spatial_window_grid_set_morph (UnitySpatialWindowGrid *self,
                                     gdouble                 morph)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_GRID (self));

  if (G_APPROX_VALUE (self->morph, morph, DBL_EPSILON))
    return;

  self->morph = morph;
  unity_spatial_window_layout_set_morph (layout_of (self), morph);
  update_chrome (self);
}

void
unity_spatial_window_grid_set_wall (UnitySpatialWindowGrid *self,
                                    gboolean                wall)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_GRID (self));

  if (self->wall == wall)
    return;

  self->wall = wall;
  update_chrome (self);
}
