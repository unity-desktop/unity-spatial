/* unity-spatial-carousel.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-carousel-private.h"

#include <math.h>

#include "unity-spatial-workspace-view-private.h"

struct _UnitySpatialCarousel
{
  GtkWidget parent_instance;

  AdwSwipeTracker *columns;
  AdwSwipeTracker *rows;
  AdwAnimation    *animation;
  GtkOrientation   axis;
  gboolean         swiping;
  gdouble          x;
  gdouble          y;
  gdouble          distance_x;
  gdouble          distance_y;
  gdouble          progress;
  gint             spacing;
  gboolean         scrolled;
};

static void unity_spatial_carousel_swipeable_init (AdwSwipeableInterface *iface);
static void update_geometry (UnitySpatialCarousel *self);

G_DEFINE_FINAL_TYPE_WITH_CODE (UnitySpatialCarousel, unity_spatial_carousel, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (ADW_TYPE_SWIPEABLE, unity_spatial_carousel_swipeable_init))

typedef enum
{
  PROP_SPACING = 1,
  PROP_SCROLLED,
} UnitySpatialCarouselProperty;

static GParamSpec *properties[PROP_SCROLLED + 1];

typedef enum
{
  SIGNAL_PAGE_CHANGED,
} UnitySpatialCarouselSignal;

static guint signals[SIGNAL_PAGE_CHANGED + 1];

static gint
n_pages (GtkOrientation axis)
{
  UnitySpatialWorkspaceView *view = unity_spatial_workspace_view_get_default ();

  return MAX (axis == GTK_ORIENTATION_HORIZONTAL ? unity_spatial_workspace_view_get_grid_width (view)
                                                 : unity_spatial_workspace_view_get_grid_height (view), 1);
}

static gdouble *
position_of (UnitySpatialCarousel *self,
             GtkOrientation          axis)
{
  return axis == GTK_ORIENTATION_HORIZONTAL ? &self->x : &self->y;
}

static void
set_position (UnitySpatialCarousel *self,
              gdouble                 position)
{
  *position_of (self, self->axis) = CLAMP (position, 0, n_pages (self->axis) - 1);
  update_geometry (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
animation_value_cb (gdouble                 value,
                    UnitySpatialCarousel *self)
{
  set_position (self, value);
}

static DexFuture *
switched_cb (DexFuture *future,
             gpointer   user_data)
{
  g_signal_emit (user_data, signals[SIGNAL_PAGE_CHANGED], 0);

  return NULL;
}

static void
animation_done_cb (UnitySpatialCarousel *self)
{
  UnitySpatialWorkspaceView *view     = unity_spatial_workspace_view_get_default ();
  UnitySpatialWorkspacePage *page     = unity_spatial_workspace_view_get_workspace (view, round (self->x), round (self->y));
  DexFuture                 *switched = NULL;

  if (page != NULL && page != unity_spatial_workspace_view_get_current (view))
    switched = unity_spatial_workspace_view_switch (view, page);
  if (switched == NULL)
    switched = dex_future_new_true ();

  dex_future_disown (dex_future_finally (switched, switched_cb, g_object_ref (self), g_object_unref));
}

static void
current_changed_cb (UnitySpatialCarousel *self)
{
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_current (unity_spatial_workspace_view_get_default ());

  if (current == NULL || self->swiping || adw_animation_get_state (self->animation) == ADW_ANIMATION_PLAYING)
    return;

  self->x = unity_spatial_workspace_page_get_x (current);
  self->y = unity_spatial_workspace_page_get_y (current);
  update_geometry (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
prepare_cb (UnitySpatialCarousel *self,
            AdwNavigationDirection  direction,
            AdwSwipeTracker        *tracker)
{
  self->axis = gtk_orientable_get_orientation (GTK_ORIENTABLE (tracker));
}

static void
begin_swipe_cb (UnitySpatialCarousel *self)
{
  unity_spatial_carousel_begin_swipe (self, self->axis);
}

static AdwSwipeTracker *
new_tracker (UnitySpatialCarousel *self,
             GtkOrientation          axis)
{
  AdwSwipeTracker *tracker = adw_swipe_tracker_new (ADW_SWIPEABLE (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (tracker), axis);
  adw_swipe_tracker_set_enabled (tracker, FALSE);
  g_signal_connect_object (tracker, "prepare", G_CALLBACK (prepare_cb), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (tracker, "begin-swipe", G_CALLBACK (begin_swipe_cb), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (tracker, "update-swipe", G_CALLBACK (unity_spatial_carousel_update_swipe), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (tracker, "end-swipe", G_CALLBACK (unity_spatial_carousel_end_swipe), self,
                           G_CONNECT_SWAPPED);

  return tracker;
}

static UnitySpatialWorkspaceThumbnail *
thumbnail_at (UnitySpatialCarousel *self,
         gint                    x,
         gint                    y)
{
  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      UnitySpatialWorkspacePage *page = unity_spatial_workspace_thumbnail_get_workspace (UNITY_SPATIAL_WORKSPACE_THUMBNAIL (child));

      if (unity_spatial_workspace_page_get_x (page) == x && unity_spatial_workspace_page_get_y (page) == y)
        return UNITY_SPATIAL_WORKSPACE_THUMBNAIL (child);
    }

  return NULL;
}

static void
update_thumbnails (UnitySpatialCarousel *self)
{
  UnitySpatialWorkspaceThumbnail *current = unity_spatial_carousel_get_current (self);
  gboolean                   wall    = self->progress > 1;
  gdouble                    morph   = CLAMP (self->progress, 0, 1);

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      UnitySpatialWorkspaceThumbnail *thumbnail = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (child);

      gtk_widget_set_can_focus (child, wall || thumbnail == current);
      if (self->progress == 2 && thumbnail == current)
        gtk_widget_set_state_flags (child, GTK_STATE_FLAG_SELECTED, FALSE);
      else
        gtk_widget_unset_state_flags (child, GTK_STATE_FLAG_SELECTED);
      unity_spatial_workspace_thumbnail_set_morph (thumbnail, thumbnail == current ? morph : morph >= 1);
      unity_spatial_workspace_thumbnail_set_wall (thumbnail, wall);
    }
}

static void
rebuild_thumbnails (UnitySpatialCarousel *self)
{
  GListModel *workspaces = G_LIST_MODEL (unity_spatial_workspace_view_get_default ());
  GtkWidget  *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))) != NULL)
    gtk_widget_unparent (child);

  for (guint i = 0; i < g_list_model_get_n_items (workspaces); i++)
    {
      g_autoptr (UnitySpatialWorkspacePage) workspace = g_list_model_get_item (workspaces, i);

      gtk_widget_set_parent (unity_spatial_workspace_thumbnail_new (workspace), GTK_WIDGET (self));
    }

  update_thumbnails (self);
  update_geometry (self);
}

typedef struct
{
  graphene_rect_t page;
  graphene_rect_t wall;
  gdouble         scale;
  gdouble         zoom;
} Layout;

static Layout
layout (UnitySpatialCarousel *self,
        gint                    width,
        gint                    height)
{
  UnitySpatialWorkspaceView *view    = unity_spatial_workspace_view_get_default ();
  const GdkRectangle        *area    = unity_spatial_workspace_view_get_workarea (view);
  gint                       columns = n_pages (GTK_ORIENTATION_HORIZONTAL);
  gint                       rows    = n_pages (GTK_ORIENTATION_VERTICAL);
  GtkWidget                 *widget  = GTK_WIDGET (self);
  GtkNative                 *native  = gtk_widget_get_native (widget);
  Layout                     layout  = { .zoom = CLAMP (self->progress - 1, 0, 1) };
  graphene_rect_t            slot;
  graphene_point_t           origin;
  gint                       output_width;
  gint                       output_height;
  gdouble                    aspect;

  unity_spatial_workspace_view_get_output_size (view, &output_width, &output_height);
  aspect = output_width > 0 && output_height > 0 ? (gdouble) output_width / output_height
                                                 : (gdouble) width / MAX (height, 1);

  graphene_rect_init (&slot, 0, 0, MIN (width, height * aspect), MIN (width, height * aspect) / aspect);
  graphene_rect_offset (&slot, (width - slot.size.width) / 2, (height - slot.size.height) / 2);

  layout.page = slot;
  if (self->progress < 1 && output_width > 0 && native != NULL &&
      gtk_widget_compute_point (widget, GTK_WIDGET (native), &GRAPHENE_POINT_INIT (0, 0), &origin))
    graphene_rect_interpolate (&GRAPHENE_RECT_INIT (-area->x - origin.x, -area->y - origin.y, output_width, output_height),
                               &slot, self->progress, &layout.page);
  layout.scale = layout.page.size.width / slot.size.width;

  layout.wall.size.width  = MIN ((width - (columns - 1) * self->spacing) / (gdouble) columns,
                                 (height - (rows - 1) * self->spacing) / (gdouble) rows * aspect);
  layout.wall.size.height = layout.wall.size.width / aspect;
  layout.wall.origin.x    = (width - columns * layout.wall.size.width - (columns - 1) * self->spacing) / 2;
  layout.wall.origin.y    = (height - rows * layout.wall.size.height - (rows - 1) * self->spacing) / 2;

  return layout;
}

static graphene_rect_t
thumbnail_rect (UnitySpatialCarousel *self,
           const Layout           *layout,
           gint                    column,
           gint                    row)
{
  const graphene_rect_t *page = &layout->page;
  const graphene_rect_t *wall = &layout->wall;
  graphene_rect_t        rect;

  graphene_rect_interpolate (
    &GRAPHENE_RECT_INIT (page->origin.x + (column - self->x) * self->distance_x * layout->scale,
                         page->origin.y + (row - self->y) * self->distance_y * layout->scale, page->size.width,
                         page->size.height),
    &GRAPHENE_RECT_INIT (wall->origin.x + column * (wall->size.width + self->spacing),
                         wall->origin.y + row * (wall->size.height + self->spacing), wall->size.width, wall->size.height),
    layout->zoom, &rect);

  return rect;
}

static void
update_geometry (UnitySpatialCarousel *self)
{
  Layout   page     = layout (self, gtk_widget_get_width (GTK_WIDGET (self)), gtk_widget_get_height (GTK_WIDGET (self)));
  gdouble  edge     = -gtk_widget_get_margin_top (GTK_WIDGET (self));
  gboolean scrolled = FALSE;

  for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      UnitySpatialWorkspacePage *workspace = unity_spatial_workspace_thumbnail_get_workspace (UNITY_SPATIAL_WORKSPACE_THUMBNAIL (child));
      graphene_rect_t            rect      = thumbnail_rect (self, &page, unity_spatial_workspace_page_get_x (workspace),
                                                             unity_spatial_workspace_page_get_y (workspace));

      gtk_widget_set_child_visible (child, self->progress >= 1 || graphene_rect_intersection (&rect, &page.page, NULL));
      scrolled |= self->progress >= 1 && rect.origin.y < edge && rect.origin.y + rect.size.height > edge;
    }

  if (scrolled == self->scrolled)
    return;

  self->scrolled = scrolled;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SCROLLED]);
}

static void
unity_spatial_carousel_size_allocate (GtkWidget *widget,
                                        gint       width,
                                        gint       height,
                                        gint       baseline)
{
  UnitySpatialCarousel *self = UNITY_SPATIAL_CAROUSEL (widget);
  Layout                  page;

  self->distance_x = width + self->spacing;
  self->distance_y = height + self->spacing;
  page             = layout (self, width, height);

  for (GtkWidget *child = gtk_widget_get_first_child (widget); child != NULL; child = gtk_widget_get_next_sibling (child))
    {
      UnitySpatialWorkspacePage *workspace = unity_spatial_workspace_thumbnail_get_workspace (UNITY_SPATIAL_WORKSPACE_THUMBNAIL (child));
      graphene_rect_t            rect      = thumbnail_rect (self, &page, unity_spatial_workspace_page_get_x (workspace),
                                                        unity_spatial_workspace_page_get_y (workspace));
      GskTransform              *transform;
      gint                       child_width;
      gint                       child_height;

      gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL, -1, &child_width, NULL, NULL, NULL);
      gtk_widget_measure (child, GTK_ORIENTATION_VERTICAL, -1, &child_height, NULL, NULL, NULL);
      child_width  = MAX (child_width, (gint) (page.page.size.width / page.scale));
      child_height = MAX (child_height, (gint) (page.page.size.height / page.scale));

      transform = gsk_transform_translate (NULL, &rect.origin);
      transform = gsk_transform_scale (transform, rect.size.width / child_width, rect.size.height / child_height);
      gtk_widget_allocate (child, child_width, child_height, -1, transform);
    }
}

static gdouble
unity_spatial_carousel_get_distance (AdwSwipeable *swipeable)
{
  UnitySpatialCarousel *self = UNITY_SPATIAL_CAROUSEL (swipeable);

  return self->axis == GTK_ORIENTATION_HORIZONTAL ? self->distance_x : self->distance_y;
}

static gdouble *
unity_spatial_carousel_get_snap_points (AdwSwipeable *swipeable,
                                          gint         *n_snap_points)
{
  gint     n      = n_pages (UNITY_SPATIAL_CAROUSEL (swipeable)->axis);
  gdouble *points = g_new (gdouble, n);

  for (gint i = 0; i < n; i++)
    points[i] = i;
  *n_snap_points = n;

  return points;
}

static gdouble
unity_spatial_carousel_get_progress (AdwSwipeable *swipeable)
{
  UnitySpatialCarousel *self = UNITY_SPATIAL_CAROUSEL (swipeable);

  return *position_of (self, self->axis);
}

static gdouble
unity_spatial_carousel_get_cancel_progress (AdwSwipeable *swipeable)
{
  return round (unity_spatial_carousel_get_progress (swipeable));
}

static void
unity_spatial_carousel_swipeable_init (AdwSwipeableInterface *iface)
{
  iface->get_distance        = unity_spatial_carousel_get_distance;
  iface->get_snap_points     = unity_spatial_carousel_get_snap_points;
  iface->get_progress        = unity_spatial_carousel_get_progress;
  iface->get_cancel_progress = unity_spatial_carousel_get_cancel_progress;
}

static void
unity_spatial_carousel_dispose (GObject *object)
{
  UnitySpatialCarousel *self = UNITY_SPATIAL_CAROUSEL (object);
  GtkWidget              *child;

  g_clear_object (&self->animation);
  g_clear_object (&self->columns);
  g_clear_object (&self->rows);
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))) != NULL)
    gtk_widget_unparent (child);
  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_CAROUSEL);

  G_OBJECT_CLASS (unity_spatial_carousel_parent_class)->dispose (object);
}

static void
unity_spatial_carousel_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  UnitySpatialCarousel *self = UNITY_SPATIAL_CAROUSEL (object);

  switch ((UnitySpatialCarouselProperty) prop_id)
    {
    case PROP_SPACING:
      g_value_set_int (value, self->spacing);
      break;
    case PROP_SCROLLED:
      g_value_set_boolean (value, self->scrolled);
      break;
    }
}

static void
unity_spatial_carousel_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  UnitySpatialCarousel *self = UNITY_SPATIAL_CAROUSEL (object);

  switch ((UnitySpatialCarouselProperty) prop_id)
    {
    case PROP_SPACING:
      self->spacing = g_value_get_int (value);
      gtk_widget_queue_allocate (GTK_WIDGET (self));
      break;
    case PROP_SCROLLED:
      break;
    }
}

static void
unity_spatial_carousel_class_init (UnitySpatialCarouselClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = unity_spatial_carousel_dispose;
  object_class->get_property = unity_spatial_carousel_get_property;
  object_class->set_property = unity_spatial_carousel_set_property;

  widget_class->size_allocate = unity_spatial_carousel_size_allocate;

  /**
   * UnitySpatialCarousel:spacing:
   *
   * The gap between two workspaces, in pixels.
   */
  properties[PROP_SPACING] =
    g_param_spec_int ("spacing", NULL, NULL, 0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialCarousel:scrolled:
   *
   * Whether a workspace crosses the top edge of the margin box, so that a bar
   * above it covers part of it.
   */
  properties[PROP_SCROLLED] =
    g_param_spec_boolean ("scrolled", NULL, NULL, FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  /**
   * UnitySpatialCarousel::page-changed:
   * @self: a #UnitySpatialCarousel
   *
   * Emitted after a slide settled and its workspace became the current one.
   */
  signals[SIGNAL_PAGE_CHANGED] =
    g_signal_new ("page-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-carousel.ui");
  gtk_widget_class_set_css_name (widget_class, "carousel");
}

static void
unity_spatial_carousel_init (UnitySpatialCarousel *self)
{
  UnitySpatialWorkspaceView *view = unity_spatial_workspace_view_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  self->columns   = new_tracker (self, GTK_ORIENTATION_HORIZONTAL);
  self->rows      = new_tracker (self, GTK_ORIENTATION_VERTICAL);
  self->animation =
    adw_spring_animation_new (GTK_WIDGET (self), 0, 0, adw_spring_params_new (1, 0.5, 500),
                              adw_callback_animation_target_new ((AdwAnimationTargetFunc) animation_value_cb, self, NULL));
  adw_spring_animation_set_clamp (ADW_SPRING_ANIMATION (self->animation), TRUE);
  g_signal_connect_swapped (self->animation, "done", G_CALLBACK (animation_done_cb), self);

  g_signal_connect_object (view, "items-changed", G_CALLBACK (rebuild_thumbnails), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (view, "notify::current", G_CALLBACK (update_thumbnails), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (view, "notify::current", G_CALLBACK (current_changed_cb), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (view, "notify::workarea", G_CALLBACK (gtk_widget_queue_allocate), self, G_CONNECT_SWAPPED);
  rebuild_thumbnails (self);
  current_changed_cb (self);
}

UnitySpatialWorkspaceThumbnail *
unity_spatial_carousel_get_current (UnitySpatialCarousel *self)
{
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_current (unity_spatial_workspace_view_get_default ());

  g_return_val_if_fail (UNITY_SPATIAL_IS_CAROUSEL (self), NULL);

  if (current == NULL)
    return NULL;

  return thumbnail_at (self, unity_spatial_workspace_page_get_x (current), unity_spatial_workspace_page_get_y (current));
}

void
unity_spatial_carousel_set_progress (UnitySpatialCarousel *self,
                                       gdouble                 progress)
{
  g_return_if_fail (UNITY_SPATIAL_IS_CAROUSEL (self));

  self->progress = progress;
  adw_swipe_tracker_set_enabled (self->columns, progress == 1);
  adw_swipe_tracker_set_enabled (self->rows, progress == 1);
  update_thumbnails (self);
  update_geometry (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

gdouble
unity_spatial_carousel_get_position (UnitySpatialCarousel *self,
                                       GtkOrientation          axis)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_CAROUSEL (self), 0);

  return *position_of (self, axis);
}

void
unity_spatial_carousel_begin_swipe (UnitySpatialCarousel *self,
                                      GtkOrientation          axis)
{
  g_return_if_fail (UNITY_SPATIAL_IS_CAROUSEL (self));

  adw_animation_pause (self->animation);
  self->axis    = axis;
  self->swiping = TRUE;
}

void
unity_spatial_carousel_update_swipe (UnitySpatialCarousel *self,
                                       gdouble                 progress)
{
  g_return_if_fail (UNITY_SPATIAL_IS_CAROUSEL (self));

  set_position (self, progress);
}

void
unity_spatial_carousel_end_swipe (UnitySpatialCarousel *self,
                                    gdouble                 velocity,
                                    gdouble                 to)
{
  g_return_if_fail (UNITY_SPATIAL_IS_CAROUSEL (self));

  self->swiping = FALSE;
  adw_spring_animation_set_value_from (ADW_SPRING_ANIMATION (self->animation), *position_of (self, self->axis));
  adw_spring_animation_set_value_to (ADW_SPRING_ANIMATION (self->animation), CLAMP (to, 0, n_pages (self->axis) - 1));
  adw_spring_animation_set_initial_velocity (ADW_SPRING_ANIMATION (self->animation), velocity);
  adw_animation_play (self->animation);
}
