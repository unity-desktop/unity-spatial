/* unity-spatial-overview.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-overview-private.h"

#include <math.h>

#include <adwaita.h>

#include "unity-spatial-stage-private.h"
#include "unity-spatial-swipe-tracker-private.h"
#include "unity-spatial-workspace-view-private.h"

#define AXIS_LOCK      8
#define SLIDE_DURATION 250

struct _UnitySpatialOverview
{
  GObject parent_instance;

  UnitySpatialStage        *stage;
  UnitySpatialWorkspaces   *workspaces;
  UnitySpatialSwipeTracker *tracker;
  AdwAnimation             *animation;
  gboolean                  tracking;
  gint                      axis_x;
  gint                      axis_y;
  gdouble                   accum_x;
  gdouble                   accum_y;
  gdouble                   slide;
};

G_DEFINE_FINAL_TYPE (UnitySpatialOverview, unity_spatial_overview, G_TYPE_OBJECT)

typedef enum
{
  SIGNAL_SLID,
} UnitySpatialOverviewSignal;

static guint signals[SIGNAL_SLID + 1];

static UnitySpatialWorkspacePage *
neighbour (UnitySpatialOverview *self,
           gint                  steps)
{
  UnitySpatialWorkspaceView *view    = unity_spatial_workspace_view_get_default ();
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_current (view);

  if (current == NULL)
    return NULL;

  return unity_spatial_workspace_view_get_workspace (view, unity_spatial_workspace_page_get_x (current) + self->axis_x * steps,
                                                     unity_spatial_workspace_page_get_y (current) + self->axis_y * steps);
}

static void
show_slide (UnitySpatialOverview *self,
            gdouble               slide)
{
  self->slide = slide;
  gtk_widget_queue_allocate (GTK_WIDGET (self->stage));
}

static void
slide_value_cb (gdouble               value,
                UnitySpatialOverview *self)
{
  show_slide (self, value);
}

static DexFuture *
switched_cb (DexFuture *future,
             gpointer   user_data)
{
  g_signal_emit (user_data, signals[SIGNAL_SLID], 0);

  return NULL;
}

static void
slide_done_cb (UnitySpatialOverview *self)
{
  UnitySpatialWorkspaceView *view     = unity_spatial_workspace_view_get_default ();
  UnitySpatialWorkspacePage *target   = self->slide != 0 ? neighbour (self, self->slide > 0 ? 1 : -1) : NULL;
  DexFuture                 *switched = NULL;

  g_object_freeze_notify (G_OBJECT (view));
  if (target != NULL)
    switched = unity_spatial_workspace_view_switch (view, target);
  self->axis_x = self->axis_y = 0;
  show_slide (self, 0);
  g_object_thaw_notify (G_OBJECT (view));

  if (switched == NULL)
    switched = dex_future_new_true ();
  dex_future_disown (dex_future_finally (switched, switched_cb, g_object_ref (self), g_object_unref));
}

static gboolean
scroll_cb (UnitySpatialOverview     *self,
           gdouble                   dx,
           gdouble                   dy,
           GtkEventControllerScroll *scroll)
{
  GdkEvent *event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (scroll));

  if (!self->tracking)
    {
      gboolean touchpad = event != NULL && gdk_event_get_event_type (event) == GDK_SCROLL &&
                          gdk_scroll_event_get_unit (event) == GDK_SCROLL_UNIT_SURFACE;

      if (unity_spatial_stage_get_progress (self->stage) != 1 || !touchpad)
        return GDK_EVENT_PROPAGATE;

      unity_spatial_overview_slide_begin (self);
    }

  unity_spatial_overview_slide_update (self, gtk_event_controller_get_current_event_time (GTK_EVENT_CONTROLLER (scroll)),
                                       dx, dy);

  return GDK_EVENT_STOP;
}

static void
scroll_end_cb (UnitySpatialOverview *self)
{
  unity_spatial_overview_slide_end (self, FALSE);
}

static void
unity_spatial_overview_dispose (GObject *object)
{
  UnitySpatialOverview *self = UNITY_SPATIAL_OVERVIEW (object);

  g_clear_pointer (&self->tracker, unity_spatial_swipe_tracker_free);
  if (self->animation != NULL)
    adw_animation_pause (self->animation);
  g_clear_object (&self->animation);
  self->stage      = NULL;
  self->workspaces = NULL;

  G_OBJECT_CLASS (unity_spatial_overview_parent_class)->dispose (object);
}

static void
unity_spatial_overview_class_init (UnitySpatialOverviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = unity_spatial_overview_dispose;

  signals[SIGNAL_SLID] =
    g_signal_new ("slid", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
unity_spatial_overview_init (UnitySpatialOverview *self)
{
}

UnitySpatialOverview *
unity_spatial_overview_new (UnitySpatialStage      *stage,
                            UnitySpatialWorkspaces *workspaces)
{
  UnitySpatialOverview *self;
  GtkEventController   *scroll;

  g_return_val_if_fail (UNITY_SPATIAL_IS_STAGE (stage), NULL);

  self            = g_object_new (UNITY_SPATIAL_TYPE_OVERVIEW, NULL);
  self->stage      = stage;
  self->workspaces = workspaces;
  self->animation =
    adw_timed_animation_new (GTK_WIDGET (stage), 0, 1, SLIDE_DURATION,
                             adw_callback_animation_target_new ((AdwAnimationTargetFunc) slide_value_cb, self, NULL));
  g_signal_connect_swapped (self->animation, "done", G_CALLBACK (slide_done_cb), self);

  scroll = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
  g_signal_connect_object (scroll, "scroll", G_CALLBACK (scroll_cb), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (scroll, "scroll-end", G_CALLBACK (scroll_end_cb), self, G_CONNECT_SWAPPED);
  gtk_widget_add_controller (GTK_WIDGET (stage), scroll);

  return self;
}

GskTransform *
unity_spatial_overview_get_zoom (UnitySpatialOverview *self,
                                 gdouble               morph)
{
  UnitySpatialWorkspaceView *view    = unity_spatial_workspace_view_get_default ();
  UnitySpatialWorkspacePage *current = unity_spatial_workspace_view_get_current (view);
  const GdkRectangle        *slot    = unity_spatial_workspaces_get_slot (self->workspaces);
  const GdkRectangle        *area    = unity_spatial_workspace_view_get_workarea (view);
  gdouble                    x       = self->axis_x * self->slide;
  gdouble                    y       = self->axis_y * self->slide;
  graphene_rect_t            overview = GRAPHENE_RECT_INIT (slot->x, slot->y, slot->width, slot->height);
  graphene_rect_t            desktop  = overview;
  graphene_rect_t            card;
  graphene_rect_t            target;
  gint                       output_width;
  gint                       output_height;

  g_return_val_if_fail (UNITY_SPATIAL_IS_OVERVIEW (self), NULL);

  if (current != NULL)
    {
      x += unity_spatial_workspace_page_get_x (current);
      y += unity_spatial_workspace_page_get_y (current);
    }

  unity_spatial_workspace_view_get_output_size (view, &output_width, &output_height);
  if (output_width > 0 && output_height > 0)
    desktop = GRAPHENE_RECT_INIT (-area->x, -area->y, output_width, output_height);

  card = unity_spatial_workspaces_get_card_rect (self->workspaces, x, y);
  graphene_rect_interpolate (&desktop, &overview, morph, &target);

  return unity_spatial_zoom_onto (&card, &target);
}

gboolean
unity_spatial_overview_slide_active (UnitySpatialOverview *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_OVERVIEW (self), FALSE);

  return self->tracking || adw_animation_get_state (self->animation) == ADW_ANIMATION_PLAYING;
}

void
unity_spatial_overview_slide_begin (UnitySpatialOverview *self)
{
  g_return_if_fail (UNITY_SPATIAL_IS_OVERVIEW (self));

  if (self->tracking)
    return;

  if (adw_animation_get_state (self->animation) == ADW_ANIMATION_PLAYING)
    adw_animation_skip (self->animation);

  self->tracking = TRUE;
  self->accum_x  = self->accum_y = 0;
}

void
unity_spatial_overview_slide_update (UnitySpatialOverview *self,
                                     guint32               time,
                                     gdouble               dx,
                                     gdouble               dy)
{
  g_return_if_fail (UNITY_SPATIAL_IS_OVERVIEW (self));

  if (!self->tracking)
    return;

  if (self->tracker == NULL)
    {
      self->accum_x += dx;
      self->accum_y += dy;
      if (MAX (fabs (self->accum_x), fabs (self->accum_y)) < AXIS_LOCK)
        return;

      self->axis_x  = fabs (self->accum_x) >= fabs (self->accum_y);
      self->axis_y  = !self->axis_x;
      self->tracker = unity_spatial_swipe_tracker_new (0, neighbour (self, -1) != NULL ? -1 : 0,
                                                       neighbour (self, 1) != NULL ? 1 : 0);
      dx            = self->accum_x;
      dy            = self->accum_y;
    }

  show_slide (self, unity_spatial_swipe_tracker_update (self->tracker, time, self->axis_x * dx + self->axis_y * dy));
}

void
unity_spatial_overview_slide_end (UnitySpatialOverview *self,
                                  gboolean              cancelled)
{
  gdouble target = 0;

  g_return_if_fail (UNITY_SPATIAL_IS_OVERVIEW (self));

  if (!self->tracking)
    return;

  self->tracking = FALSE;
  if (self->tracker != NULL)
    target = unity_spatial_swipe_tracker_end (self->tracker, cancelled);
  g_clear_pointer (&self->tracker, unity_spatial_swipe_tracker_free);

  adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (self->animation), self->slide);
  adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (self->animation), target);
  adw_animation_play (self->animation);
}

void
unity_spatial_overview_slide_reset (UnitySpatialOverview *self)
{
  g_return_if_fail (UNITY_SPATIAL_IS_OVERVIEW (self));

  self->tracking = FALSE;
  g_clear_pointer (&self->tracker, unity_spatial_swipe_tracker_free);
  adw_animation_pause (self->animation);
  self->axis_x = self->axis_y = 0;
  show_slide (self, 0);
}
