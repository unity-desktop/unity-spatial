/* unity-spatial-mirror-stack.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-mirror-stack-private.h"

#include <gdk/wayland/gdkwayland.h>

#include "unity-spatial-preview-mirror-private.h"

struct _UnitySpatialMirrorStack
{
  GObject parent_instance;

  GtkNative     *native;
  GdkFrameClock *clock;
  GPtrArray     *mirrors;
};

G_DEFINE_FINAL_TYPE (UnitySpatialMirrorStack, unity_spatial_mirror_stack, G_TYPE_OBJECT)

G_DEFINE_QUARK (unity-spatial-mirror-stack, stack)

static void
collect (GtkWidget *widget,
         GPtrArray *mirrors,
         GPtrArray *ordered)
{
  if (g_ptr_array_find (mirrors, widget, NULL))
    g_ptr_array_add (ordered, widget);

  for (GtkWidget *child = gtk_widget_get_first_child (widget); child != NULL;
       child = gtk_widget_get_next_sibling (child))
    collect (child, mirrors, ordered);
}

static gboolean
in_order (GPtrArray *mirrors,
          GPtrArray *ordered)
{
  for (guint i = 0; i < ordered->len; i++)
    if (g_ptr_array_index (ordered, i) != g_ptr_array_index (mirrors, i))
      return FALSE;

  return TRUE;
}

static gboolean
restack (UnitySpatialMirrorStack *self)
{
  g_autoptr (GPtrArray) ordered = g_ptr_array_new ();
  struct wl_surface    *parent;

  collect (GTK_WIDGET (self->native), self->mirrors, ordered);

  if (ordered->len != self->mirrors->len || in_order (self->mirrors, ordered))
    return FALSE;

  parent = gdk_wayland_surface_get_wl_surface (gtk_native_get_surface (self->native));
  for (guint i = 0; i < ordered->len; i++)
    unity_spatial_preview_mirror_place_below (g_ptr_array_index (ordered, i), parent);

  g_ptr_array_unref (self->mirrors);
  self->mirrors = g_steal_pointer (&ordered);

  return TRUE;
}

static void
layout_cb (UnitySpatialMirrorStack *self)
{
  gboolean changed = restack (self);

  for (guint i = 0; i < self->mirrors->len; i++)
    changed |= unity_spatial_preview_mirror_sync (g_ptr_array_index (self->mirrors, i));

  if (changed)
    gtk_widget_queue_draw (GTK_WIDGET (self->native));
}

static void
unity_spatial_mirror_stack_dispose (GObject *object)
{
  UnitySpatialMirrorStack *self = UNITY_SPATIAL_MIRROR_STACK (object);

  if (self->clock != NULL)
    g_signal_handlers_disconnect_by_data (self->clock, self);
  g_clear_object (&self->clock);
  g_clear_pointer (&self->mirrors, g_ptr_array_unref);

  G_OBJECT_CLASS (unity_spatial_mirror_stack_parent_class)->dispose (object);
}

static void
unity_spatial_mirror_stack_class_init (UnitySpatialMirrorStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = unity_spatial_mirror_stack_dispose;
}

static void
unity_spatial_mirror_stack_init (UnitySpatialMirrorStack *self)
{
  self->mirrors = g_ptr_array_new ();
}

UnitySpatialMirrorStack *
unity_spatial_mirror_stack_get_for_native (GtkNative *native)
{
  UnitySpatialMirrorStack *self;

  g_return_val_if_fail (GTK_IS_NATIVE (native), NULL);

  self = g_object_get_qdata (G_OBJECT (native), stack_quark ());
  if (self != NULL)
    return g_object_ref (self);

  self         = g_object_new (UNITY_SPATIAL_TYPE_MIRROR_STACK, NULL);
  self->native = native;
  self->clock  = g_object_ref (gtk_widget_get_frame_clock (GTK_WIDGET (native)));
  g_signal_connect_swapped (self->clock, "layout", G_CALLBACK (layout_cb), self);
  g_object_set_qdata_full (G_OBJECT (native), stack_quark (), g_object_ref (self), g_object_unref);

  return self;
}

void
unity_spatial_mirror_stack_add (UnitySpatialMirrorStack   *self,
                                UnitySpatialPreviewMirror *mirror)
{
  g_return_if_fail (UNITY_SPATIAL_IS_MIRROR_STACK (self));
  g_return_if_fail (UNITY_SPATIAL_IS_PREVIEW_MIRROR (mirror));

  g_ptr_array_add (self->mirrors, mirror);
  gdk_frame_clock_request_phase (self->clock, GDK_FRAME_CLOCK_PHASE_LAYOUT);
}

void
unity_spatial_mirror_stack_remove (UnitySpatialMirrorStack   *self,
                                   UnitySpatialPreviewMirror *mirror)
{
  g_return_if_fail (UNITY_SPATIAL_IS_MIRROR_STACK (self));

  g_ptr_array_remove (self->mirrors, mirror);

  if (self->mirrors->len == 0)
    g_object_set_qdata (G_OBJECT (self->native), stack_quark (), NULL);
  else
    gtk_widget_queue_draw (GTK_WIDGET (self->native));
}
