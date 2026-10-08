/* unity-spatial-gesture.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-gesture-private.h"

#include "unity-spatial-globals-private.h"

struct _UnitySpatialGesture
{
  GObject parent_instance;

  struct zunity_spatial_gesture *gesture;
};

G_DEFINE_FINAL_TYPE (UnitySpatialGesture, unity_spatial_gesture, G_TYPE_OBJECT)

typedef enum
{
  SIGNAL_BEGIN,
  SIGNAL_UPDATE,
  SIGNAL_END,
  N_SIGNALS,
} UnitySpatialGestureSignal;

static guint signals[N_SIGNALS];

static void
gesture_begin (gpointer                       data,
               struct zunity_spatial_gesture *gesture,
               guint32                        time)
{
  g_signal_emit (data, signals[SIGNAL_BEGIN], 0, time);
}

static void
gesture_update (gpointer                       data,
                struct zunity_spatial_gesture *gesture,
                guint32                        time,
                wl_fixed_t                     dx,
                wl_fixed_t                     dy,
                wl_fixed_t                     scale,
                wl_fixed_t                     rotation)
{
  g_signal_emit (data, signals[SIGNAL_UPDATE], 0, time, wl_fixed_to_double (dx), wl_fixed_to_double (dy),
                 wl_fixed_to_double (scale), wl_fixed_to_double (rotation));
}

static void
gesture_end (gpointer                       data,
             struct zunity_spatial_gesture *gesture,
             guint32                        time,
             gint32                         cancelled)
{
  g_signal_emit (data, signals[SIGNAL_END], 0, time, cancelled != 0);
}

static const struct zunity_spatial_gesture_listener listener = {
  .begin  = gesture_begin,
  .update = gesture_update,
  .end    = gesture_end,
};

static void
unity_spatial_gesture_dispose (GObject *object)
{
  UnitySpatialGesture *self = UNITY_SPATIAL_GESTURE (object);

  g_clear_pointer (&self->gesture, zunity_spatial_gesture_destroy);

  G_OBJECT_CLASS (unity_spatial_gesture_parent_class)->dispose (object);
}

static void
unity_spatial_gesture_class_init (UnitySpatialGestureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = unity_spatial_gesture_dispose;

  signals[SIGNAL_BEGIN] =
    g_signal_new ("begin", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_UINT);
  signals[SIGNAL_UPDATE] =
    g_signal_new ("update", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                  G_TYPE_NONE, 5, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  signals[SIGNAL_END] =
    g_signal_new ("end", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_BOOLEAN);
}

static void
unity_spatial_gesture_init (UnitySpatialGesture *self)
{
}

static UnitySpatialGesture *
gesture_new (guint32 kind,
             guint   fingers)
{
  struct zunity_spatial_gestures *manager = unity_spatial_globals_get ()->gestures;
  UnitySpatialGesture            *self    = g_object_new (UNITY_SPATIAL_TYPE_GESTURE, NULL);

  if (manager != NULL)
    {
      self->gesture = zunity_spatial_gestures_get_gesture (manager, kind, fingers);
      zunity_spatial_gesture_add_listener (self->gesture, &listener, self);
    }

  return self;
}

UnitySpatialGesture *
unity_spatial_gesture_new_swipe (guint fingers)
{
  return gesture_new (ZUNITY_SPATIAL_GESTURES_KIND_SWIPE, fingers);
}

UnitySpatialGesture *
unity_spatial_gesture_new_pinch (guint fingers)
{
  return gesture_new (ZUNITY_SPATIAL_GESTURES_KIND_PINCH, fingers);
}
