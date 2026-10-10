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

  GtkStack     *stack;
  GtkPicture   *front;
  GtkPicture   *back;
  GdkTexture   *pending;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWallpaperWindow, unity_spatial_wallpaper_window, UNITY_TYPE_WINDOW)

static GtkPicture *
hidden_picture (UnitySpatialWallpaperWindow *self)
{
  return gtk_stack_get_visible_child (self->stack) == GTK_WIDGET (self->front) ? self->back : self->front;
}

static void
show_texture (UnitySpatialWallpaperWindow *self,
              GdkTexture                  *texture)
{
  GtkPicture *shown   = GTK_PICTURE (gtk_stack_get_visible_child (self->stack));
  GtkPicture *next    = hidden_picture (self);
  gboolean    animate = GDK_IS_TEXTURE (gtk_picture_get_paintable (shown));

  gtk_picture_set_paintable (next, GDK_PAINTABLE (texture));
  gtk_stack_set_visible_child_full (self->stack, next == self->front ? "front" : "back",
                                    animate ? GTK_STACK_TRANSITION_TYPE_CROSSFADE : GTK_STACK_TRANSITION_TYPE_NONE);
  if (!gtk_stack_get_transition_running (self->stack))
    gtk_picture_set_paintable (hidden_picture (self), NULL);
}

static void
transition_running_cb (UnitySpatialWallpaperWindow *self)
{
  g_autoptr (GdkTexture) pending = NULL;

  if (gtk_stack_get_transition_running (self->stack))
    return;

  gtk_picture_set_paintable (hidden_picture (self), NULL);
  pending = g_steal_pointer (&self->pending);
  if (pending != NULL)
    show_texture (self, pending);
}

static void
texture_changed_cb (UnitySpatialWallpaperWindow *self,
                    GParamSpec                  *pspec,
                    UnityBackgroundsSource      *source)
{
  GdkTexture *texture = unity_backgrounds_source_get_texture (source);

  if (gtk_stack_get_transition_running (self->stack))
    g_set_object (&self->pending, texture);
  else
    show_texture (self, texture);
}

static void
unity_spatial_wallpaper_window_dispose (GObject *object)
{
  UnitySpatialWallpaperWindow *self = UNITY_SPATIAL_WALLPAPER_WINDOW (object);

  g_clear_object (&self->pending);
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
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWallpaperWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWallpaperWindow, front);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWallpaperWindow, back);
  gtk_widget_class_bind_template_callback (widget_class, transition_running_cb);
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
  gtk_picture_set_paintable (self->front, GDK_PAINTABLE (source));
  g_signal_connect_object (source, "notify::texture", G_CALLBACK (texture_changed_cb), self, G_CONNECT_SWAPPED);
  if (unity_backgrounds_source_get_texture (source) != NULL)
    texture_changed_cb (self, NULL, source);

  return self;
}
