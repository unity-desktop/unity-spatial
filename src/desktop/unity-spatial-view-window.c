/* unity-spatial-view-window.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-view-window-private.h"

struct _UnitySpatialViewWindow
{
  UnityWindow parent_instance;

  UnitySpatialView *view;
};

G_DEFINE_FINAL_TYPE (UnitySpatialViewWindow, unity_spatial_view_window, UNITY_TYPE_WINDOW)

static void
unity_spatial_view_window_dispose (GObject *object)
{
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_SPATIAL_TYPE_VIEW_WINDOW);

  G_OBJECT_CLASS (unity_spatial_view_window_parent_class)->dispose (object);
}

static void
unity_spatial_view_window_class_init (UnitySpatialViewWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_spatial_view_window_dispose;

  g_type_ensure (UNITY_SPATIAL_TYPE_VIEW);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-view-window.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialViewWindow, view);
}

static void
unity_spatial_view_window_init (UnitySpatialViewWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

UnitySpatialViewWindow *
unity_spatial_view_window_new (GtkApplication *app,
                               GdkMonitor     *monitor)
{
  g_return_val_if_fail (GTK_IS_APPLICATION (app), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return g_object_new (UNITY_SPATIAL_TYPE_VIEW_WINDOW, "application", app, "gdkmonitor", monitor, NULL);
}

UnitySpatialView *
unity_spatial_view_window_get_view (UnitySpatialViewWindow *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_VIEW_WINDOW (self), NULL);

  return self->view;
}
