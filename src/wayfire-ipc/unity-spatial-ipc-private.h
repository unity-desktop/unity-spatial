/* unity-spatial-ipc-private.h
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <json-glib/json-glib.h>
#include <libdex.h>

G_BEGIN_DECLS

#define UNITY_SPATIAL_TYPE_IPC (unity_spatial_ipc_get_type ())

G_DECLARE_FINAL_TYPE (UnitySpatialIpc, unity_spatial_ipc, UNITY_SPATIAL, IPC, GObject)

UnitySpatialIpc *unity_spatial_ipc_new   (void);

DexFuture       *unity_spatial_ipc_call  (UnitySpatialIpc     *self,
                                          const gchar         *method,
                                          JsonObject          *data);

void             unity_spatial_ipc_send  (UnitySpatialIpc     *self,
                                          const gchar         *method,
                                          JsonObject          *data);

void             unity_spatial_ipc_watch (UnitySpatialIpc     *self,
                                          const gchar * const *events);

G_END_DECLS
