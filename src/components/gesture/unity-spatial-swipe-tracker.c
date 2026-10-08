/* unity-spatial-swipe-tracker.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-swipe-tracker-private.h"

#include <math.h>

#define DISTANCE   300.0
#define FLING      0.6
#define HISTORY_MS 150

typedef struct
{
  guint32 time;
  gdouble delta;
} Sample;

struct _UnitySpatialSwipeTracker
{
  gdouble from;
  gdouble lower;
  gdouble upper;
  gdouble value;
  GArray *samples;
};

UnitySpatialSwipeTracker *
unity_spatial_swipe_tracker_new (gdouble from,
                         gdouble lower,
                         gdouble upper)
{
  UnitySpatialSwipeTracker *self = g_new0 (UnitySpatialSwipeTracker, 1);

  self->lower   = lower;
  self->upper   = upper;
  self->from    = CLAMP (from, lower, upper);
  self->value   = self->from;
  self->samples = g_array_new (FALSE, FALSE, sizeof (Sample));

  return self;
}

void
unity_spatial_swipe_tracker_free (UnitySpatialSwipeTracker *self)
{
  g_array_unref (self->samples);
  g_free (self);
}

gdouble
unity_spatial_swipe_tracker_update (UnitySpatialSwipeTracker *self,
                            guint32            time,
                            gdouble            delta)
{
  g_array_append_val (self->samples, ((Sample) { time, delta }));
  while (self->samples->len > 1 && time - g_array_index (self->samples, Sample, 0).time > HISTORY_MS)
    g_array_remove_index (self->samples, 0);

  self->value = CLAMP (self->value + delta / DISTANCE, self->lower, self->upper);

  return self->value;
}

static gdouble
velocity (UnitySpatialSwipeTracker *self)
{
  guint32 period;
  gdouble sum = 0;

  if (self->samples->len < 2)
    return 0;

  period = g_array_index (self->samples, Sample, self->samples->len - 1).time -
           g_array_index (self->samples, Sample, 0).time;
  if (period == 0)
    return 0;

  for (guint i = 1; i < self->samples->len; i++)
    sum += g_array_index (self->samples, Sample, i).delta;

  return sum / period;
}

gdouble
unity_spatial_swipe_tracker_end (UnitySpatialSwipeTracker *self,
                         gboolean           cancelled)
{
  gdouble speed = velocity (self);
  gdouble target;

  if (cancelled)
    target = round (self->from);
  else if (speed > FLING)
    target = floor (self->value) + 1;
  else if (speed < -FLING)
    target = ceil (self->value) - 1;
  else
    target = round (self->value);

  return CLAMP (target, self->lower, self->upper);
}
