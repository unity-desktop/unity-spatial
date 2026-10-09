/* unity-spatial-desktop.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-desktop.h"

#include <math.h>

#include <adwaita.h>

#include "unity-spatial-enums.h"
#include "unity-spatial-gesture-private.h"
#include "unity-spatial-swipe-tracker-private.h"
#include "unity-spatial-view-window-private.h"
#include "unity-spatial-wallpaper-window-private.h"
#include "unity-spatial-wayfire-private.h"
#include "unity-spatial-workspace-view.h"

#define PAGE_FINGERS             3
#define SLIDE_FINGERS            4
#define PINCH_SCALE              0.25
#define AXIS_LOCK                8
#define TOUCHPAD_BASE_DISTANCE_H 400
#define TOUCHPAD_BASE_DISTANCE_V 300

struct _UnitySpatialDesktop
{
  GObject parent_instance;

  GtkApplication           *app;
  UnityBackgroundsSource   *source;
  GSettings                *settings;
  GPtrArray                *wallpapers;
  UnitySpatialViewWindow   *window;
  UnitySpatialSwipeTracker *swipe;
  UnitySpatialSwipeTracker *slide;
  GtkOrientation            slide_axis;
  gboolean                  sliding;
  gdouble                   slide_x;
  gdouble                   slide_y;
  gboolean                  swipe_pending;
  UnitySpatialGesture      *page_swipe;
  UnitySpatialGesture      *page_pinch;
  UnitySpatialGesture      *slide_swipe;
  gdouble                   pinch_scale;
  UnitySpatialPage          page;
};

G_DEFINE_FINAL_TYPE (UnitySpatialDesktop, unity_spatial_desktop, G_TYPE_OBJECT)

typedef enum
{
  PROP_PAGE = 1,
  PROP_VIEW_VISIBLE,
} UnitySpatialDesktopProperty;

static GParamSpec *properties[PROP_VIEW_VISIBLE + 1];

static UnitySpatialView *
view_of (UnitySpatialDesktop *self)
{
  return unity_spatial_view_window_get_view (self->window);
}

static gboolean
shown (UnitySpatialDesktop *self)
{
  return self->window != NULL && gtk_widget_get_visible (GTK_WIDGET (self->window));
}

static GdkMonitor *
find_monitor (void)
{
  const gchar *name     = unity_spatial_workspace_view_get_output_name (unity_spatial_workspace_view_get_default ());
  GListModel  *monitors = gdk_display_get_monitors (gdk_display_get_default ());
  GdkMonitor  *first    = NULL;

  for (guint i = 0; i < g_list_model_get_n_items (monitors); i++)
    {
      g_autoptr (GdkMonitor) monitor = g_list_model_get_item (monitors, i);

      if (first == NULL)
        first = monitor;
      if (g_strcmp0 (gdk_monitor_get_connector (monitor), name) == 0)
        return monitor;
    }

  return first;
}

static void
hide_window (UnitySpatialDesktop *self)
{
  if (!shown (self))
    return;

  g_clear_pointer (&self->swipe, unity_spatial_swipe_tracker_free);
  g_clear_pointer (&self->slide, unity_spatial_swipe_tracker_free);
  self->sliding = FALSE;
  unity_spatial_view_set_progress (view_of (self), UNITY_SPATIAL_PAGE_DESKTOP);
  gtk_widget_set_visible (GTK_WIDGET (self->window), FALSE);
  unity_spatial_wayfire_set_active (unity_spatial_wayfire_get_default (), FALSE);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VIEW_VISIBLE]);
}

static void
destroy_window (UnitySpatialDesktop *self)
{
  if (self->window != NULL)
    gtk_window_destroy (GTK_WINDOW (g_steal_pointer (&self->window)));
}

static void
view_page_cb (UnitySpatialDesktop *self)
{
  UnitySpatialPage page = unity_spatial_view_get_page (view_of (self));

  if (self->page == page)
    return;

  self->page = page;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAGE]);
}

static gboolean
show_window (UnitySpatialDesktop *self)
{
  GdkMonitor       *monitor = find_monitor ();
  UnitySpatialView *view;

  if (monitor == NULL)
    return FALSE;

  unity_spatial_wayfire_set_active (unity_spatial_wayfire_get_default (), TRUE);

  if (self->window != NULL && unity_window_get_gdkmonitor (UNITY_WINDOW (self->window)) != monitor)
    destroy_window (self);
  if (self->window == NULL)
    {
      self->window = unity_spatial_view_window_new (self->app, monitor);
      view         = view_of (self);
      g_settings_bind (self->settings, "show-search", view, "enable-search", G_SETTINGS_BIND_GET);
      g_signal_connect_object (view, "closed", G_CALLBACK (hide_window), self, G_CONNECT_SWAPPED);
      g_signal_connect_object (view, "notify::page", G_CALLBACK (view_page_cb), self, G_CONNECT_SWAPPED);
    }

  gtk_window_present (GTK_WINDOW (self->window));
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VIEW_VISIBLE]);

  return TRUE;
}

static void
start_page_swipe (UnitySpatialDesktop *self)
{
  gdouble from = unity_spatial_view_get_progress (view_of (self));

  self->swipe = unity_spatial_swipe_tracker_new (from, MAX (UNITY_SPATIAL_PAGE_DESKTOP, round (from) - 1),
                                                 MIN (UNITY_SPATIAL_PAGE_WORKSPACES, round (from) + 1),
                                                 TOUCHPAD_BASE_DISTANCE_V);
  unity_spatial_view_begin_swipe (view_of (self));
}

static void
page_swipe_begin_cb (UnitySpatialDesktop *self)
{
  if (self->swipe != NULL || self->sliding)
    return;

  if (shown (self))
    start_page_swipe (self);
  else
    self->swipe_pending = TRUE;
}

static void
page_swipe_update_cb (UnitySpatialDesktop *self,
                      guint                time,
                      gdouble              dx,
                      gdouble              dy)
{
  if (self->swipe_pending && !G_APPROX_VALUE (dy, 0, DBL_EPSILON))
    {
      self->swipe_pending = FALSE;
      if (dy < 0 && show_window (self))
        start_page_swipe (self);
    }

  if (self->swipe != NULL)
    unity_spatial_view_update_swipe (view_of (self), unity_spatial_swipe_tracker_update (self->swipe, time, -dy));
}

static void
page_swipe_end_cb (UnitySpatialDesktop *self,
                   guint                time,
                   gboolean             cancelled)
{
  gdouble velocity;
  gdouble to;

  self->swipe_pending = FALSE;
  if (self->swipe == NULL)
    return;

  to = unity_spatial_swipe_tracker_end (self->swipe, cancelled, &velocity);
  g_clear_pointer (&self->swipe, unity_spatial_swipe_tracker_free);
  unity_spatial_view_end_swipe (view_of (self), velocity, to);
}

static void
page_pinch_begin_cb (UnitySpatialDesktop *self)
{
  self->pinch_scale = 1;
}

static void
page_pinch_update_cb (UnitySpatialDesktop *self,
                      guint                time,
                      gdouble              dx,
                      gdouble              dy,
                      gdouble              scale)
{
  self->pinch_scale = scale;
}

static void
page_pinch_end_cb (UnitySpatialDesktop *self,
                   guint                time,
                   gboolean             cancelled)
{
  if (cancelled || self->swipe != NULL || fabs (self->pinch_scale - 1) < PINCH_SCALE)
    return;

  unity_spatial_desktop_set_page (self, self->page == UNITY_SPATIAL_PAGE_WORKSPACES ? UNITY_SPATIAL_PAGE_DESKTOP
                                                                                    : UNITY_SPATIAL_PAGE_WORKSPACES);
}

static void
slide_swipe_begin_cb (UnitySpatialDesktop *self)
{
  if (self->sliding || self->swipe != NULL || self->page == UNITY_SPATIAL_PAGE_WORKSPACES ||
      (!shown (self) && !show_window (self)))
    return;

  self->sliding = TRUE;
  self->slide_x = self->slide_y = 0;
}

static void
slide_swipe_update_cb (UnitySpatialDesktop *self,
                       guint                time,
                       gdouble              dx,
                       gdouble              dy)
{
  UnitySpatialWorkspaceView *view     = unity_spatial_workspace_view_get_default ();
  UnitySpatialCarousel      *carousel;
  gdouble                    position;
  gint                       pages;

  if (!self->sliding)
    return;

  carousel = unity_spatial_view_get_carousel (view_of (self));
  if (self->slide == NULL)
    {
      self->slide_x -= dx;
      self->slide_y -= dy;
      if (MAX (fabs (self->slide_x), fabs (self->slide_y)) < AXIS_LOCK)
        return;

      self->slide_axis = fabs (self->slide_x) >= fabs (self->slide_y) ? GTK_ORIENTATION_HORIZONTAL
                                                                       : GTK_ORIENTATION_VERTICAL;
      pages            = self->slide_axis == GTK_ORIENTATION_HORIZONTAL ? unity_spatial_workspace_view_get_grid_width (view)
                                                                        : unity_spatial_workspace_view_get_grid_height (view);
      position         = unity_spatial_carousel_get_position (carousel, self->slide_axis);
      self->slide      = unity_spatial_swipe_tracker_new (position, MAX (position - 1, 0), MIN (position + 1, pages - 1),
                                                         self->slide_axis == GTK_ORIENTATION_HORIZONTAL ? TOUCHPAD_BASE_DISTANCE_H
                                                                                                        : TOUCHPAD_BASE_DISTANCE_V);
      unity_spatial_carousel_begin_swipe (carousel, self->slide_axis);
      dx = -self->slide_x;
      dy = -self->slide_y;
    }

  unity_spatial_carousel_update_swipe (carousel,
                                       unity_spatial_swipe_tracker_update (self->slide, time,
                                                                           self->slide_axis == GTK_ORIENTATION_HORIZONTAL
                                                                           ? -dx : -dy));
}

static void
slide_swipe_end_cb (UnitySpatialDesktop *self,
                    guint                time,
                    gboolean             cancelled)
{
  gdouble velocity;
  gdouble to;

  if (!self->sliding)
    return;

  self->sliding = FALSE;
  if (self->slide == NULL)
    {
      if (self->page == UNITY_SPATIAL_PAGE_DESKTOP && unity_spatial_view_get_progress (view_of (self)) == 0)
        hide_window (self);
      return;
    }

  to = unity_spatial_swipe_tracker_end (self->slide, cancelled, &velocity);
  g_clear_pointer (&self->slide, unity_spatial_swipe_tracker_free);
  unity_spatial_carousel_end_swipe (unity_spatial_view_get_carousel (view_of (self)), velocity, to);
}

static void
monitors_changed_cb (UnitySpatialDesktop *self,
                     guint                position,
                     guint                removed,
                     guint                added,
                     GListModel          *monitors)
{
  g_ptr_array_remove_range (self->wallpapers, position, removed);

  for (guint i = 0; i < added; i++)
    {
      g_autoptr (GdkMonitor)      monitor = g_list_model_get_item (monitors, position + i);
      UnitySpatialWallpaperWindow *window = unity_spatial_wallpaper_window_new (self->app, monitor, self->source);

      g_ptr_array_insert (self->wallpapers, position + i, window);
      gtk_window_present (GTK_WINDOW (window));
    }
}

static void
connect_gesture (UnitySpatialDesktop *self,
                 UnitySpatialGesture *gesture,
                 GCallback            begin,
                 GCallback            update,
                 GCallback            end)
{
  g_signal_connect_object (gesture, "begin", begin, self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gesture, "update", update, self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gesture, "end", end, self, G_CONNECT_SWAPPED);
}

static void
unity_spatial_desktop_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  UnitySpatialDesktop *self = UNITY_SPATIAL_DESKTOP (object);

  switch ((UnitySpatialDesktopProperty) prop_id)
    {
    case PROP_PAGE:
      g_value_set_enum (value, self->page);
      break;
    case PROP_VIEW_VISIBLE:
      g_value_set_boolean (value, shown (self));
      break;
    }
}

static void
unity_spatial_desktop_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  UnitySpatialDesktop *self = UNITY_SPATIAL_DESKTOP (object);

  switch ((UnitySpatialDesktopProperty) prop_id)
    {
    case PROP_PAGE:
      unity_spatial_desktop_set_page (self, g_value_get_enum (value));
      break;
    case PROP_VIEW_VISIBLE:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
unity_spatial_desktop_dispose (GObject *object)
{
  UnitySpatialDesktop *self = UNITY_SPATIAL_DESKTOP (object);

  hide_window (self);
  destroy_window (self);
  g_clear_object (&self->page_swipe);
  g_clear_object (&self->page_pinch);
  g_clear_object (&self->slide_swipe);
  g_clear_pointer (&self->wallpapers, g_ptr_array_unref);
  g_clear_object (&self->source);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (unity_spatial_desktop_parent_class)->dispose (object);
}

static void
unity_spatial_desktop_class_init (UnitySpatialDesktopClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = unity_spatial_desktop_dispose;
  object_class->get_property = unity_spatial_desktop_get_property;
  object_class->set_property = unity_spatial_desktop_set_property;

  properties[PROP_PAGE] =
    g_param_spec_enum ("page", NULL, NULL, UNITY_TYPE_SPATIAL_PAGE, UNITY_SPATIAL_PAGE_DESKTOP,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_VIEW_VISIBLE] =
    g_param_spec_boolean ("view-visible", NULL, NULL, FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_spatial_desktop_init (UnitySpatialDesktop *self)
{
  self->wallpapers = g_ptr_array_new_with_free_func ((GDestroyNotify) gtk_window_destroy);
}

UnitySpatialDesktop *
unity_spatial_desktop_new (GtkApplication *app)
{
  UnitySpatialDesktop *self;
  GListModel          *monitors;

  g_return_val_if_fail (GTK_IS_APPLICATION (app), NULL);

  self              = g_object_new (UNITY_SPATIAL_TYPE_DESKTOP, NULL);
  self->app         = app;
  self->source      = unity_backgrounds_source_new ();
  self->settings    = g_settings_new ("org.unity.spatial");
  self->page_swipe  = unity_spatial_gesture_new_swipe (PAGE_FINGERS);
  self->page_pinch  = unity_spatial_gesture_new_pinch (PAGE_FINGERS);
  self->slide_swipe = unity_spatial_gesture_new_swipe (SLIDE_FINGERS);
  unity_spatial_wayfire_set_active (unity_spatial_wayfire_get_default (), FALSE);

  connect_gesture (self, self->page_swipe, G_CALLBACK (page_swipe_begin_cb), G_CALLBACK (page_swipe_update_cb),
                   G_CALLBACK (page_swipe_end_cb));
  connect_gesture (self, self->page_pinch, G_CALLBACK (page_pinch_begin_cb), G_CALLBACK (page_pinch_update_cb),
                   G_CALLBACK (page_pinch_end_cb));
  connect_gesture (self, self->slide_swipe, G_CALLBACK (slide_swipe_begin_cb), G_CALLBACK (slide_swipe_update_cb),
                   G_CALLBACK (slide_swipe_end_cb));

  monitors = gdk_display_get_monitors (gdk_display_get_default ());
  g_signal_connect_object (monitors, "items-changed", G_CALLBACK (monitors_changed_cb), self, G_CONNECT_SWAPPED);
  monitors_changed_cb (self, 0, 0, g_list_model_get_n_items (monitors), monitors);

  return self;
}

UnitySpatialPage
unity_spatial_desktop_get_page (UnitySpatialDesktop *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_DESKTOP (self), UNITY_SPATIAL_PAGE_DESKTOP);

  return self->page;
}

void
unity_spatial_desktop_set_page (UnitySpatialDesktop *self,
                                UnitySpatialPage     page)
{
  g_return_if_fail (UNITY_SPATIAL_IS_DESKTOP (self));

  if (self->page == page)
    return;

  g_clear_pointer (&self->swipe, unity_spatial_swipe_tracker_free);
  if (shown (self) || (page != UNITY_SPATIAL_PAGE_DESKTOP && show_window (self)))
    unity_spatial_view_set_page (view_of (self), page);
}

gboolean
unity_spatial_desktop_get_view_visible (UnitySpatialDesktop *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_DESKTOP (self), FALSE);

  return shown (self);
}
