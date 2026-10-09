/* unity-spatial-workspaces.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-workspaces-private.h"

#include <math.h>

#include "unity-spatial-workspace-view.h"

struct _UnitySpatialWorkspaces
{
  GtkGrid parent_instance;

  gdouble       progress;
  GdkRectangle  slot;
  gint          gap;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWorkspaces, unity_spatial_workspaces, GTK_TYPE_GRID)

GskTransform *
unity_spatial_zoom_onto (const graphene_rect_t *from,
                         const graphene_rect_t *to)
{
  GskTransform *transform = gsk_transform_translate (NULL, &to->origin);

  transform = gsk_transform_scale (transform, to->size.width / from->size.width, to->size.height / from->size.height);

  return gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (-from->origin.x, -from->origin.y));
}

static gboolean
card_bounds (UnitySpatialWorkspaces *self,
             gint                    x,
             gint                    y,
             graphene_rect_t        *rect)
{
  GtkWidget *card = gtk_grid_get_child_at (GTK_GRID (self), x, y);

  if (card == NULL || !gtk_widget_compute_bounds (card, GTK_WIDGET (self), rect))
    return FALSE;

  graphene_rect_offset (rect, gtk_widget_get_margin_start (GTK_WIDGET (self)),
                        gtk_widget_get_margin_top (GTK_WIDGET (self)));

  return TRUE;
}

static void
grid_size (UnitySpatialWorkspaces *self,
           gint                   *columns,
           gint                   *rows)
{
  UnitySpatialWorkspaceView *view = unity_spatial_workspace_view_get_default ();

  *columns = MAX (unity_spatial_workspace_view_get_grid_width (view), 1);
  *rows    = MAX (unity_spatial_workspace_view_get_grid_height (view), 1);
}

static void
update_cards (UnitySpatialWorkspaces *self)
{
  UnitySpatialWorkspaceCard *current = unity_spatial_workspaces_get_current_card (self);
  gboolean                   wall    = self->progress > 1;
  gdouble                    morph   = CLAMP (self->progress, 0, 1);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      UnitySpatialWorkspaceCard *card = UNITY_SPATIAL_WORKSPACE_CARD (child);

      gtk_widget_set_can_focus (child, wall || card == current);
      unity_spatial_workspace_card_set_morph (card, card == current ? morph : morph >= 1);
      unity_spatial_workspace_card_set_wall (card, wall);
    }
}

static void
rebuild_cards (UnitySpatialWorkspaces *self)
{
  GListModel *workspaces = G_LIST_MODEL (unity_spatial_workspace_view_get_default ());

  GtkWidget  *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))) != NULL)
    gtk_grid_remove (GTK_GRID (self), child);

  for (guint i = 0; i < g_list_model_get_n_items (workspaces); i++)
    {
      g_autoptr (UnitySpatialWorkspacePage) workspace = g_list_model_get_item (workspaces, i);
      GtkWidget                            *card      = unity_spatial_workspace_card_new (workspace);

      gtk_widget_set_size_request (card, self->slot.width, self->slot.height);
      gtk_grid_attach (GTK_GRID (self), card, unity_spatial_workspace_page_get_x (workspace),
                       unity_spatial_workspace_page_get_y (workspace), 1, 1);
    }

  update_cards (self);
}

static void
unity_spatial_workspaces_dispose (GObject *object)
{
  UnitySpatialWorkspaces *self = UNITY_SPATIAL_WORKSPACES (object);

  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_WORKSPACES);

  G_OBJECT_CLASS (unity_spatial_workspaces_parent_class)->dispose (object);
}

static void
unity_spatial_workspaces_class_init (UnitySpatialWorkspacesClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_spatial_workspaces_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-workspaces.ui");
}

static void
unity_spatial_workspaces_init (UnitySpatialWorkspaces *self)
{
  UnitySpatialWorkspaceView *view = unity_spatial_workspace_view_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  self->gap = gtk_grid_get_column_spacing (GTK_GRID (self));

  g_signal_connect_object (view, "items-changed", G_CALLBACK (rebuild_cards), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (view, "notify::current", G_CALLBACK (update_cards), self, G_CONNECT_SWAPPED);
  rebuild_cards (self);
}

UnitySpatialWorkspaceCard *
unity_spatial_workspaces_get_current_card (UnitySpatialWorkspaces *self)
{
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_current (unity_spatial_workspace_view_get_default ());

  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACES (self), NULL);

  if (current == NULL)
    return NULL;

  return UNITY_SPATIAL_WORKSPACE_CARD (gtk_grid_get_child_at (GTK_GRID (self), unity_spatial_workspace_page_get_x (current),
                                                              unity_spatial_workspace_page_get_y (current)));
}

void
unity_spatial_workspaces_set_progress (UnitySpatialWorkspaces *self,
                                       gdouble                 progress)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACES (self));

  self->progress = progress;
  update_cards (self);
}

void
unity_spatial_workspaces_set_area (UnitySpatialWorkspaces *self,
                                   gint                    width,
                                   gint                    height)
{
  GtkWidget   *widget      = GTK_WIDGET (self);
  gint         start       = gtk_widget_get_margin_start (widget);
  gint         top         = gtk_widget_get_margin_top (widget);
  gint         area_width  = MAX (width - start - gtk_widget_get_margin_end (widget), 1);
  gint         area_height = MAX (height - top - gtk_widget_get_margin_bottom (widget), 1);
  GdkRectangle slot        = { start, top, area_width, area_height };
  gint         output_width;
  gint         output_height;
  gint         columns;
  gint         rows;
  gint         spacing;
  gdouble      scale;

  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACES (self));

  unity_spatial_workspace_view_get_output_size (unity_spatial_workspace_view_get_default (), &output_width,
                                                &output_height);
  if (output_width > 0 && output_height > 0)
    {
      scale       = MIN ((gdouble) area_width / output_width, (gdouble) area_height / output_height);
      slot.width  = MAX ((gint) (output_width * scale), 1);
      slot.height = MAX ((gint) (output_height * scale), 1);
      slot.x      = start + (area_width - slot.width) / 2;
      slot.y      = top + (area_height - slot.height) / 2;
    }

  if (gdk_rectangle_equal (&slot, &self->slot))
    return;

  self->slot = slot;
  grid_size (self, &columns, &rows);
  scale         = MIN ((gdouble) (slot.width - (columns - 1) * self->gap) / (columns * slot.width),
                       (gdouble) (slot.height - (rows - 1) * self->gap) / (rows * slot.height));
  spacing = scale > 0 ? (gint) round (self->gap / scale) : self->gap;
  gtk_grid_set_column_spacing (GTK_GRID (self), spacing);
  gtk_grid_set_row_spacing (GTK_GRID (self), spacing);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    gtk_widget_set_size_request (child, slot.width, slot.height);
}

const GdkRectangle *
unity_spatial_workspaces_get_slot (UnitySpatialWorkspaces *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACES (self), NULL);

  return &self->slot;
}

graphene_rect_t
unity_spatial_workspaces_get_card_rect (UnitySpatialWorkspaces *self,
                                        gdouble                 x,
                                        gdouble                 y)
{
  gint            column = floor (x);
  gint            row    = floor (y);
  graphene_rect_t rect   = GRAPHENE_RECT_INIT (self->slot.x, self->slot.y, self->slot.width, self->slot.height);
  graphene_rect_t next;

  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACES (self), rect);

  if (!card_bounds (self, column, row, &rect))
    return rect;
  if (x > column && card_bounds (self, column + 1, row, &next))
    rect.origin.x += (x - column) * (next.origin.x - rect.origin.x);
  if (y > row && card_bounds (self, column, row + 1, &next))
    rect.origin.y += (y - row) * (next.origin.y - rect.origin.y);

  return rect;
}

GskTransform *
unity_spatial_workspaces_get_zoom (UnitySpatialWorkspaces *self,
                                   gdouble                 zoom)
{
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_current (unity_spatial_workspace_view_get_default ());
  GtkWidget                 *widget  = GTK_WIDGET (self);
  graphene_rect_t            slot    = GRAPHENE_RECT_INIT (self->slot.x, self->slot.y, self->slot.width,
                                                           self->slot.height);
  graphene_rect_t            card;
  graphene_rect_t            wall;
  graphene_rect_t            target;
  gdouble                    scale;

  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACES (self), NULL);

  card  = unity_spatial_workspaces_get_card_rect (self, current != NULL ? unity_spatial_workspace_page_get_x (current) : 0,
                                                  current != NULL ? unity_spatial_workspace_page_get_y (current) : 0);
  scale = MIN (slot.size.width / gtk_widget_get_width (widget), slot.size.height / gtk_widget_get_height (widget));
  wall  = GRAPHENE_RECT_INIT (slot.origin.x + (slot.size.width - gtk_widget_get_width (widget) * scale) / 2 +
                              (card.origin.x - gtk_widget_get_margin_start (widget)) * scale,
                              slot.origin.y + (slot.size.height - gtk_widget_get_height (widget) * scale) / 2 +
                              (card.origin.y - gtk_widget_get_margin_top (widget)) * scale,
                              card.size.width * scale, card.size.height * scale);
  graphene_rect_interpolate (&slot, &wall, zoom, &target);

  return unity_spatial_zoom_onto (&card, &target);
}
