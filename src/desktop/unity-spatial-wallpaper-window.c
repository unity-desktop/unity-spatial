/* unity-spatial-wallpaper-window.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-wallpaper-window-private.h"

struct _UnitySpatialWallpaperWindow
{
  UnityWindow parent_instance;

  GtkPicture *wallpaper;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWallpaperWindow, unity_spatial_wallpaper_window, UNITY_TYPE_WINDOW)

static void
unity_spatial_wallpaper_window_dispose (GObject *object)
{
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_SPATIAL_TYPE_WALLPAPER_WINDOW);

  G_OBJECT_CLASS (unity_spatial_wallpaper_window_parent_class)->dispose (object);
}

static void
unity_spatial_wallpaper_window_class_init (UnitySpatialWallpaperWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_spatial_wallpaper_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-wallpaper-window.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWallpaperWindow, wallpaper);
}

static void
unity_spatial_wallpaper_window_init (UnitySpatialWallpaperWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

UnitySpatialWallpaperWindow *
unity_spatial_wallpaper_window_new (GtkApplication         *app,
                                   GdkMonitor             *monitor,
                                   UnityBackgroundsSource *source)
{
  UnitySpatialWallpaperWindow *self;

  g_return_val_if_fail (GTK_IS_APPLICATION (app), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
  g_return_val_if_fail (UNITY_BACKGROUNDS_IS_SOURCE (source), NULL);

  self = g_object_new (UNITY_SPATIAL_TYPE_WALLPAPER_WINDOW, "application", app, "gdkmonitor", monitor, NULL);
  gtk_picture_set_paintable (self->wallpaper, GDK_PAINTABLE (source));

  return self;
}
