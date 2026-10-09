/* unity-spatial-window-layout.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-window-layout-private.h"

#include <adwaita.h>

#include "unity-spatial-preview-mirror-private.h"
#include "unity-spatial-window-thumbnail-private.h"
#include "unity-spatial-workspace-view.h"

#define SPACING        24
#define MAX_PREVIEW    0.95
#define SMALL_BOOST    1.5
#define SPACE_WEIGHT   0.1
#define APPEAR_SCALE   0.75
#define REFLOW_DAMPING 0.8
#define REFLOW_STIFF   400

#define UNITY_SPATIAL_TYPE_WINDOW_LAYOUT_CHILD (unity_spatial_window_layout_child_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialWindowLayoutChild, unity_spatial_window_layout_child, UNITY_SPATIAL,
                      WINDOW_LAYOUT_CHILD, GtkLayoutChild)

struct _UnitySpatialWindowLayoutChild
{
  GtkLayoutChild parent_instance;

  graphene_rect_t from;
  graphene_rect_t to;
  GdkRectangle    frame;
  gboolean        placed;
  gboolean        appearing;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWindowLayoutChild, unity_spatial_window_layout_child, GTK_TYPE_LAYOUT_CHILD)

struct _UnitySpatialWindowLayout
{
  GtkLayoutManager parent_instance;

  AdwAnimation *reflow;
  gdouble       settle;
  gdouble       morph;
  gint          width;
  gint          height;
  guint         n_placed;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWindowLayout, unity_spatial_window_layout, GTK_TYPE_LAYOUT_MANAGER)

typedef struct
{
  UnitySpatialWindowLayoutChild *child;
  const GdkRectangle            *frame;
  gdouble                        boost;
} Item;

typedef struct
{
  guint   start;
  guint   count;
  gdouble width;
  gdouble height;
} Row;

static void
unity_spatial_window_layout_child_class_init (UnitySpatialWindowLayoutChildClass *klass)
{
}

static void
unity_spatial_window_layout_child_init (UnitySpatialWindowLayoutChild *self)
{
}

static UnitySpatialWindowLayoutChild *
child_of (UnitySpatialWindowLayout *self,
          GtkWidget                *widget)
{
  return UNITY_SPATIAL_WINDOW_LAYOUT_CHILD (gtk_layout_manager_get_layout_child (GTK_LAYOUT_MANAGER (self), widget));
}

static const GdkRectangle *
frame_of (UnitySpatialWindowLayoutChild *child)
{
  GtkWidget *widget = gtk_layout_child_get_child_widget (GTK_LAYOUT_CHILD (child));

  return unity_spatial_window_page_get_bounds (
    unity_spatial_window_thumbnail_get_page (UNITY_SPATIAL_WINDOW_THUMBNAIL (widget)));
}

static gint
compare_center_y (gconstpointer a,
                  gconstpointer b,
                  gpointer      user_data)
{
  const GdkRectangle *frame_a = ((const Item *) a)->frame;
  const GdkRectangle *frame_b = ((const Item *) b)->frame;

  return (frame_a->y * 2 + frame_a->height) - (frame_b->y * 2 + frame_b->height);
}

static gint
compare_center_x (gconstpointer a,
                  gconstpointer b,
                  gpointer      user_data)
{
  const GdkRectangle *frame_a = ((const Item *) a)->frame;
  const GdkRectangle *frame_b = ((const Item *) b)->frame;

  return (frame_a->x * 2 + frame_a->width) - (frame_b->x * 2 + frame_b->width);
}

static GArray *
pack_rows (GArray  *items,
           guint    n_rows,
           gdouble  total_width)
{
  GArray *rows  = g_array_new (FALSE, TRUE, sizeof (Row));
  guint   index = 0;

  for (guint r = 0; r < n_rows && index < items->len; r++)
    {
      Row row = { index, 0, 0, 0 };

      for (; index < items->len; index++)
        {
          Item   *item   = &g_array_index (items, Item, index);
          gdouble width  = item->frame->width * item->boost;
          gdouble ideal  = total_width / n_rows;
          gboolean keep  = row.width + width <= ideal ||
                           ABS (1 - (row.width + width) / ideal) < ABS (1 - row.width / ideal);

          if (!keep && r < n_rows - 1)
            break;

          row.count++;
          row.width  += width;
          row.height  = MAX (row.height, item->frame->height * item->boost);
        }

      g_array_append_val (rows, row);
    }

  return rows;
}

static void
place_items (GArray                *items,
             const graphene_rect_t *area,
             gdouble                gap)
{
  gdouble          total_width = 0;
  gdouble          best_scale  = 0;
  gdouble          best_score  = -1;
  g_autoptr (GArray) best      = NULL;
  gdouble          grid_height = 0;
  gdouble          y;

  g_array_sort_with_data (items, compare_center_y, NULL);
  for (guint i = 0; i < items->len; i++)
    total_width += g_array_index (items, Item, i).frame->width * g_array_index (items, Item, i).boost;

  for (guint n_rows = 1; n_rows <= items->len; n_rows++)
    {
      g_autoptr (GArray) rows     = pack_rows (items, n_rows, total_width);
      gdouble            grid_w   = 0;
      gdouble            grid_h   = 0;
      guint              max_cols = 0;
      gdouble            hspace;
      gdouble            vspace;
      gdouble            scale;
      gdouble            score;

      for (guint r = 0; r < rows->len; r++)
        {
          grid_w    = MAX (grid_w, g_array_index (rows, Row, r).width);
          grid_h   += g_array_index (rows, Row, r).height;
          max_cols  = MAX (max_cols, g_array_index (rows, Row, r).count);
        }

      hspace = (max_cols - 1) * gap;
      vspace = (rows->len - 1) * gap;
      scale  = MIN (MIN (MAX (1, area->size.width - hspace) / grid_w, MAX (1, area->size.height - vspace) / grid_h),
                    MAX_PREVIEW);
      score  = scale + SPACE_WEIGHT * (grid_w * scale + hspace) * (grid_h * scale + vspace) /
                                      (area->size.width * area->size.height);

      if (score > best_score)
        {
          g_clear_pointer (&best, g_array_unref);
          best       = g_steal_pointer (&rows);
          best_scale = scale;
          best_score = score;
        }
    }

  for (guint r = 0; r < best->len; r++)
    grid_height += g_array_index (best, Row, r).height;
  y = area->origin.y + MAX (0, (area->size.height - grid_height * best_scale - (best->len - 1) * gap) / 2);

  for (guint r = 0; r < best->len; r++)
    {
      Row    *row        = &g_array_index (best, Row, r);
      gdouble row_height = row->height * best_scale;
      gdouble x          = area->origin.x +
                           MAX (0, (area->size.width - row->width * best_scale - (row->count - 1) * gap) / 2);

      g_sort_array (&g_array_index (items, Item, row->start), row->count, sizeof (Item), compare_center_x, NULL);

      for (guint i = row->start; i < row->start + row->count; i++)
        {
          Item   *item   = &g_array_index (items, Item, i);
          gdouble scale  = MIN (best_scale * item->boost, MAX_PREVIEW);
          gdouble cell   = item->frame->width * item->boost * best_scale;
          gdouble width  = item->frame->width * scale;
          gdouble height = item->frame->height * scale;
          gdouble top    = best->len == 1 ? y + (row_height - height) / 2 : y + row_height - height;

          item->child->to = GRAPHENE_RECT_INIT (x + (cell - width) / 2, top, width, height);
          x += cell + gap;
        }

      y += row_height + gap;
    }
}

static graphene_rect_t
desktop_rect (UnitySpatialWindowLayoutChild *child,
              const graphene_rect_t         *card,
              const graphene_size_t         *output)
{
  const GdkRectangle *frame = frame_of (child);
  gdouble             sx    = card->size.width / output->width;
  gdouble             sy    = card->size.height / output->height;

  return GRAPHENE_RECT_INIT (card->origin.x + frame->x * sx, card->origin.y + frame->y * sy, frame->width * sx,
                             frame->height * sy);
}

static graphene_rect_t
current_rect (UnitySpatialWindowLayout      *self,
              UnitySpatialWindowLayoutChild *child)
{
  graphene_rect_t rect;
  gdouble         scale = child->appearing ? APPEAR_SCALE + (1 - APPEAR_SCALE) * self->settle : 1;

  graphene_rect_interpolate (&child->from, &child->to, self->settle, &rect);
  graphene_rect_inset (&rect, rect.size.width * (1 - scale) / 2, rect.size.height * (1 - scale) / 2);

  return rect;
}

static void
settle_cb (gdouble                   value,
           UnitySpatialWindowLayout *self)
{
  self->settle = value;
  gtk_widget_queue_allocate (gtk_layout_manager_get_widget (GTK_LAYOUT_MANAGER (self)));
}

static void
retarget (UnitySpatialWindowLayout *self,
          GtkWidget                *widget,
          GPtrArray                *children,
          gint                      width,
          gint                      height)
{
  gboolean           animate = gtk_widget_get_mapped (widget) && self->morph >= 1 &&
                               width == self->width && height == self->height;
  gboolean           moved   = FALSE;
  g_autoptr (GArray) items   = g_array_new (FALSE, FALSE, sizeof (Item));
  g_autoptr (GArray) shown   = g_array_new (FALSE, FALSE, sizeof (graphene_rect_t));
  graphene_rect_t    area    = GRAPHENE_RECT_INIT (0, 0, width, height);
  gint               output_width;
  gint               monitor = 0;

  self->width    = width;
  self->height   = height;
  self->n_placed = children->len;
  if (area.size.width <= 0 || area.size.height <= 0)
    return;

  unity_spatial_workspace_view_get_output_size (unity_spatial_workspace_view_get_default (), &output_width, &monitor);
  for (guint i = 0; i < children->len; i++)
    {
      UnitySpatialWindowLayoutChild *child = g_ptr_array_index (children, i);
      gdouble                        ratio = CLAMP (frame_of (child)->height / (gdouble) MAX (monitor, 1), 0, 1);
      graphene_rect_t                now   = current_rect (self, child);

      g_array_append_val (items, ((Item) { child, frame_of (child), SMALL_BOOST + (1 - SMALL_BOOST) * ratio }));
      g_array_append_val (shown, now);
      child->from = child->to;
    }

  place_items (items, &area, SPACING);

  for (guint i = 0; i < children->len; i++)
    {
      UnitySpatialWindowLayoutChild *child = g_ptr_array_index (children, i);

      moved |= !child->placed || !graphene_rect_equal (&child->from, &child->to);
    }

  for (guint i = 0; i < children->len; i++)
    {
      UnitySpatialWindowLayoutChild *child = g_ptr_array_index (children, i);

      child->frame     = *frame_of (child);
      child->appearing = animate && moved && !child->placed;
      child->from      = animate && moved && child->placed ? g_array_index (shown, graphene_rect_t, i) : child->to;
      child->placed    = TRUE;
    }

  if (animate && moved)
    {
      self->settle = 0;
      adw_animation_play (self->reflow);
    }
  else if (!animate)
    {
      adw_animation_skip (self->reflow);
    }
}

static gboolean
changed (UnitySpatialWindowLayout *self,
         GPtrArray                *children,
         gint                      width,
         gint                      height)
{
  if (width != self->width || height != self->height || children->len != self->n_placed)
    return TRUE;

  for (guint i = 0; i < children->len; i++)
    {
      UnitySpatialWindowLayoutChild *child = g_ptr_array_index (children, i);

      if (!child->placed || !gdk_rectangle_equal (&child->frame, frame_of (child)))
        return TRUE;
    }

  return FALSE;
}

static void
unity_spatial_window_layout_measure (GtkLayoutManager *manager,
                                     GtkWidget        *widget,
                                     GtkOrientation    orientation,
                                     gint              for_size,
                                     gint             *minimum,
                                     gint             *natural,
                                     gint             *minimum_baseline,
                                     gint             *natural_baseline)
{
  *minimum = *natural = 0;
}

static void
unity_spatial_window_layout_allocate (GtkLayoutManager *manager,
                                      GtkWidget        *widget,
                                      gint              width,
                                      gint              height,
                                      gint              baseline)
{
  UnitySpatialWindowLayout *self     = UNITY_SPATIAL_WINDOW_LAYOUT (manager);
  g_autoptr (GPtrArray)     children = g_ptr_array_new ();
  gboolean                  morphing = FALSE;
  graphene_rect_t           card;
  graphene_size_t           output;
  gint                      output_width;
  gint                      output_height;

  for (GtkWidget *child = gtk_widget_get_first_child (widget); child != NULL; child = gtk_widget_get_next_sibling (child))
    {
      if (gtk_widget_should_layout (child))
        g_ptr_array_add (children, child_of (self, child));
    }

  if (children->len == 0)
    return;


  if (changed (self, children, width, height))
    retarget (self, widget, children, width, height);

  unity_spatial_workspace_view_get_output_size (unity_spatial_workspace_view_get_default (), &output_width,
                                                &output_height);
  if (self->morph < 1 && output_width > 0 && output_height > 0 &&
      gtk_widget_compute_bounds (gtk_widget_get_parent (widget), widget, &card))
    {
      output   = GRAPHENE_SIZE_INIT (output_width, output_height);
      morphing = TRUE;
    }

  for (guint i = 0; i < children->len; i++)
    {
      UnitySpatialWindowLayoutChild *child = g_ptr_array_index (children, i);
      GtkWidget                     *thumb = gtk_layout_child_get_child_widget (GTK_LAYOUT_CHILD (child));
      graphene_rect_t                rect  = current_rect (self, child);
      GdkRectangle                   box;

      if (morphing)
        {
          graphene_rect_t desktop = desktop_rect (child, &card, &output);

          graphene_rect_interpolate (&desktop, &rect, self->morph, &rect);
        }

      box = unity_spatial_preview_mirror_snap_rect (&rect);
      gtk_widget_allocate (thumb, box.width, box.height, -1,
                           gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (box.x, box.y)));
    }
}

static void
unity_spatial_window_layout_dispose (GObject *object)
{
  UnitySpatialWindowLayout *self = UNITY_SPATIAL_WINDOW_LAYOUT (object);

  if (self->reflow != NULL)
    adw_animation_pause (self->reflow);
  g_clear_object (&self->reflow);

  G_OBJECT_CLASS (unity_spatial_window_layout_parent_class)->dispose (object);
}

static void
unity_spatial_window_layout_root (GtkLayoutManager *manager)
{
  UnitySpatialWindowLayout *self = UNITY_SPATIAL_WINDOW_LAYOUT (manager);

  if (self->reflow != NULL)
    return;

  self->reflow =
    adw_spring_animation_new (gtk_layout_manager_get_widget (manager), 0, 1,
                              adw_spring_params_new (REFLOW_DAMPING, 1, REFLOW_STIFF),
                              adw_callback_animation_target_new ((AdwAnimationTargetFunc) settle_cb, self, NULL));
}

static void
unity_spatial_window_layout_class_init (UnitySpatialWindowLayoutClass *klass)
{
  GObjectClass          *object_class = G_OBJECT_CLASS (klass);
  GtkLayoutManagerClass *layout_class = GTK_LAYOUT_MANAGER_CLASS (klass);

  object_class->dispose = unity_spatial_window_layout_dispose;

  layout_class->layout_child_type = UNITY_SPATIAL_TYPE_WINDOW_LAYOUT_CHILD;
  layout_class->measure           = unity_spatial_window_layout_measure;
  layout_class->allocate          = unity_spatial_window_layout_allocate;
  layout_class->root              = unity_spatial_window_layout_root;
}

static void
unity_spatial_window_layout_init (UnitySpatialWindowLayout *self)
{
  self->settle = 1;
  self->morph  = 1;
}

void
unity_spatial_window_layout_set_morph (UnitySpatialWindowLayout *self,
                                       gdouble                   morph)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_LAYOUT (self));

  if (G_APPROX_VALUE (self->morph, morph, DBL_EPSILON))
    return;

  self->morph = morph;
  gtk_widget_queue_allocate (gtk_layout_manager_get_widget (GTK_LAYOUT_MANAGER (self)));
}

void
unity_spatial_window_layout_fly_from (UnitySpatialWindowLayout *self,
                                      GtkWidget                *child,
                                      gdouble                   dx,
                                      gdouble                   dy)
{
  UnitySpatialWindowLayoutChild *info;

  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_LAYOUT (self));

  for (GtkWidget *sibling = gtk_widget_get_first_child (gtk_layout_manager_get_widget (GTK_LAYOUT_MANAGER (self)));
       sibling != NULL; sibling = gtk_widget_get_next_sibling (sibling))
    {
      UnitySpatialWindowLayoutChild *other = child_of (self, sibling);

      other->from      = current_rect (self, other);
      other->appearing = FALSE;
    }

  info = child_of (self, child);
  graphene_rect_offset (&info->from, dx, dy);
  self->settle = 0;
  adw_animation_play (self->reflow);
}
