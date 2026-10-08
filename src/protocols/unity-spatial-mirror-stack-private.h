/* unity-spatial-mirror-stack-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "unity-spatial-preview-mirror.h"

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_MIRROR_STACK (unity_spatial_mirror_stack_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialMirrorStack, unity_spatial_mirror_stack, UNITY_SPATIAL, MIRROR_STACK, GObject)

UnitySpatialMirrorStack *unity_spatial_mirror_stack_get_for_native (GtkNative                 *native);

void                     unity_spatial_mirror_stack_add            (UnitySpatialMirrorStack   *self,
                                                                    UnitySpatialPreviewMirror *mirror);

void                     unity_spatial_mirror_stack_remove         (UnitySpatialMirrorStack   *self,
                                                                    UnitySpatialPreviewMirror *mirror);

G_END_DECLS
