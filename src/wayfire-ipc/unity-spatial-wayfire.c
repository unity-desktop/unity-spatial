/* unity-spatial-wayfire.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-wayfire-private.h"

#include <math.h>
#include <unistd.h>

#include "unity-spatial-ipc-private.h"
#include "unity-spatial-window-view-private.h"
#include "unity-spatial-workspace-view-private.h"

#define DESKTOP_APP_ID "unity-desktop"

struct _UnitySpatialWayfire
{
  GObject parent_instance;

  UnitySpatialIpc           *ipc;
  UnitySpatialWindowView    *windows;
  UnitySpatialWorkspaceView *workspaces;
  gint64                     output_id;
  gboolean                   refreshing;
  gboolean                   stale;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWayfire, unity_spatial_wayfire, G_TYPE_OBJECT)

static const gchar * const watched_events[] = {
  "view-mapped",
  "view-unmapped",
  "view-title-changed",
  "view-app-id-changed",
  "view-geometry-changed",
  "view-workspace-changed",
  "view-minimized",
  "view-focused",
  "view-set-output",
  "wset-workspace-changed",
  "output-layout-changed",
  NULL,
};

static GdkRectangle
rectangle_of (JsonObject  *object,
              const gchar *member)
{
  JsonObject *rect = json_object_get_object_member (object, member);

  return (GdkRectangle) {
    json_object_get_int_member_with_default (rect, "x", 0),
    json_object_get_int_member_with_default (rect, "y", 0),
    json_object_get_int_member_with_default (rect, "width", 0),
    json_object_get_int_member_with_default (rect, "height", 0),
  };
}

static UnitySpatialOutputState
read_output (JsonObject *output)
{
  JsonObject  *workspace = json_object_get_object_member (output, "workspace");
  GdkRectangle geometry  = rectangle_of (output, "geometry");

  return (UnitySpatialOutputState) {
    .name        = json_object_get_string_member_with_default (output, "name", NULL),
    .width       = geometry.width,
    .height      = geometry.height,
    .workarea    = rectangle_of (output, "workarea"),
    .grid_width  = json_object_get_int_member (workspace, "grid_width"),
    .grid_height = json_object_get_int_member (workspace, "grid_height"),
    .x           = json_object_get_int_member (workspace, "x"),
    .y           = json_object_get_int_member (workspace, "y"),
  };
}

static gboolean
on_output (JsonObject *view,
           gint64      output_id)
{
  return json_object_get_boolean_member_with_default (view, "mapped", FALSE) &&
         json_object_get_int_member_with_default (view, "output-id", -1) == output_id;
}

static gboolean
is_window (JsonObject *view,
           gint64      output_id)
{
  return g_strcmp0 (json_object_get_string_member_with_default (view, "role", NULL), "toplevel") == 0 &&
         on_output (view, output_id) &&
         json_object_get_int_member_with_default (view, "pid", -1) != getpid ();
}

static gboolean
is_desktop (JsonObject *view,
            gint64      output_id)
{
  return g_strcmp0 (json_object_get_string_member_with_default (view, "app-id", NULL), DESKTOP_APP_ID) == 0 &&
         on_output (view, output_id);
}

static gint
workspace_of (gint center,
              gint size,
              gint current,
              gint count)
{
  gint offset = size > 0 ? (gint) floor ((gdouble) center / size) : 0;

  return CLAMP (current + offset, 0, MAX (count - 1, 0));
}

static UnitySpatialWindowState
read_window (JsonObject                    *view,
             const UnitySpatialOutputState *output)
{
  GdkRectangle geometry = rectangle_of (view, "geometry");
  gint         x        = workspace_of (geometry.x + geometry.width / 2, output->width, output->x, output->grid_width);
  gint         y        = workspace_of (geometry.y + geometry.height / 2, output->height, output->y,
                                        output->grid_height);

  return (UnitySpatialWindowState) {
    .view_id     = json_object_get_int_member (view, "id"),
    .app_id      = json_object_get_string_member_with_default (view, "app-id", NULL),
    .title       = json_object_get_string_member_with_default (view, "title", NULL),
    .bounds      = {
      geometry.x - (x - output->x) * output->width,
      geometry.y - (y - output->y) * output->height,
      geometry.width,
      geometry.height,
    },
    .workspace_x = x,
    .workspace_y = y,
    .minimized   = json_object_get_boolean_member_with_default (view, "minimized", FALSE),
  };
}

static GHashTable *
read_stacking (JsonNode *reply)
{
  GHashTable *ranks = g_hash_table_new (NULL, NULL);
  JsonObject *object;
  JsonArray  *views;

  if (reply == NULL || !JSON_NODE_HOLDS_OBJECT (reply))
    return ranks;

  object = json_node_get_object (reply);
  views  = json_object_has_member (object, "views") ? json_object_get_array_member (object, "views") : NULL;

  for (guint i = 0; views != NULL && i < json_array_get_length (views); i++)
    g_hash_table_insert (ranks, GINT_TO_POINTER (json_array_get_int_element (views, i)), GINT_TO_POINTER (i + 1));

  return ranks;
}

static gint
by_stacking (gconstpointer a,
             gconstpointer b,
             gpointer      user_data)
{
  GHashTable *ranks  = user_data;
  JsonObject *view_a = *(JsonObject **) a;
  JsonObject *view_b = *(JsonObject **) b;
  gint        rank_a = GPOINTER_TO_INT (g_hash_table_lookup (ranks, GINT_TO_POINTER (json_object_get_int_member (view_a, "id"))));
  gint        rank_b = GPOINTER_TO_INT (g_hash_table_lookup (ranks, GINT_TO_POINTER (json_object_get_int_member (view_b, "id"))));
  gint64      time_a = json_object_get_int_member_with_default (view_a, "last-focus-timestamp", 0);
  gint64      time_b = json_object_get_int_member_with_default (view_b, "last-focus-timestamp", 0);

  if (rank_a != rank_b)
    return rank_a - rank_b;

  return time_a < time_b ? 1 : time_a > time_b ? -1 : 0;
}

static void
apply (UnitySpatialWayfire *self,
       JsonObject          *output_object,
       JsonArray           *views,
       JsonNode            *stacking)
{
  UnitySpatialOutputState output  = read_output (output_object);
  g_autoptr (GPtrArray)   found   = g_ptr_array_new ();
  g_autoptr (GArray)      windows = g_array_new (FALSE, FALSE, sizeof (UnitySpatialWindowState));
  g_autoptr (GHashTable)  ranks   = read_stacking (stacking);

  for (guint i = 0; i < json_array_get_length (views); i++)
    {
      JsonObject *view = json_array_get_object_element (views, i);

      if (is_window (view, self->output_id))
        g_ptr_array_add (found, view);
      else if (is_desktop (view, self->output_id))
        output.desktop_view_id = json_object_get_int_member (view, "id");
    }

  g_ptr_array_sort_with_data (found, by_stacking, ranks);

  for (guint i = 0; i < found->len; i++)
    {
      UnitySpatialWindowState state = read_window (g_ptr_array_index (found, i), &output);

      g_array_append_val (windows, state);
    }

  unity_spatial_workspace_view_update (self->workspaces, &output);
  unity_spatial_window_view_update (self->windows, windows);
}

static gboolean
read_compositor (UnitySpatialWayfire *self)
{
  g_autoptr (DexFuture)  outputs_call = unity_spatial_ipc_call (self->ipc, "window-rules/list-outputs", NULL);
  g_autoptr (DexFuture)  views_call   = unity_spatial_ipc_call (self->ipc, "window-rules/list-views", NULL);
  g_autoptr (JsonObject) request      = json_object_new ();
  g_autoptr (JsonNode)   outputs      = NULL;
  g_autoptr (JsonNode)   views        = NULL;
  g_autoptr (JsonNode)   stacking     = NULL;
  g_autoptr (GError)     error        = NULL;
  JsonObject            *output;

  if (!dex_await (dex_future_all_race (dex_ref (outputs_call), dex_ref (views_call), NULL), &error))
    {
      g_warning ("Cannot read the windows from wayfire: %s", error->message);
      return FALSE;
    }

  outputs = dex_await_boxed (g_steal_pointer (&outputs_call), NULL);
  views   = dex_await_boxed (g_steal_pointer (&views_call), NULL);

  if (!JSON_NODE_HOLDS_ARRAY (outputs) || !JSON_NODE_HOLDS_ARRAY (views))
    {
      g_warning ("Cannot read the windows from wayfire: bad reply");
      return FALSE;
    }

  if (json_array_get_length (json_node_get_array (outputs)) == 0)
    return TRUE;

  output          = json_array_get_object_element (json_node_get_array (outputs), 0);
  self->output_id = json_object_get_int_member (output, "id");
  json_object_set_int_member (request, "output-id", self->output_id);
  stacking = dex_await_boxed (unity_spatial_ipc_call (self->ipc, "unity-spatial-preview/stacking", request), NULL);

  apply (self, output, json_node_get_array (views), stacking);

  return TRUE;
}

static DexFuture *
refresh_fiber (gpointer user_data)
{
  UnitySpatialWayfire *self = user_data;

  while (self->stale)
    {
      self->stale = FALSE;
      if (!read_compositor (self))
        break;
    }

  self->refreshing = FALSE;

  return dex_future_new_true ();
}

static void
send_view_request (UnitySpatialWayfire *self,
                   const gchar         *method,
                   guint                view_id)
{
  g_autoptr (JsonObject) data = json_object_new ();

  json_object_set_int_member (data, "id", view_id);
  unity_spatial_ipc_send (self->ipc, method, data);
}

static void
unity_spatial_wayfire_constructed (GObject *object)
{
  UnitySpatialWayfire *self = UNITY_SPATIAL_WAYFIRE (object);

  G_OBJECT_CLASS (unity_spatial_wayfire_parent_class)->constructed (object);

  g_signal_connect_object (self->ipc, "event", G_CALLBACK (unity_spatial_wayfire_refresh), self, G_CONNECT_SWAPPED);
  unity_spatial_ipc_watch (self->ipc, watched_events);
  unity_spatial_wayfire_refresh (self);
}

static void
unity_spatial_wayfire_dispose (GObject *object)
{
  UnitySpatialWayfire *self = UNITY_SPATIAL_WAYFIRE (object);

  g_clear_object (&self->ipc);
  g_clear_object (&self->windows);
  g_clear_object (&self->workspaces);

  G_OBJECT_CLASS (unity_spatial_wayfire_parent_class)->dispose (object);
}

static void
unity_spatial_wayfire_class_init (UnitySpatialWayfireClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = unity_spatial_wayfire_constructed;
  object_class->dispose     = unity_spatial_wayfire_dispose;
}

static void
unity_spatial_wayfire_init (UnitySpatialWayfire *self)
{
  self->ipc        = unity_spatial_ipc_new ();
  self->windows    = g_object_new (UNITY_SPATIAL_TYPE_WINDOW_VIEW, NULL);
  self->workspaces = g_object_new (UNITY_SPATIAL_TYPE_WORKSPACE_VIEW, NULL);
  self->output_id  = -1;
}

UnitySpatialWayfire *
unity_spatial_wayfire_get_default (void)
{
  static UnitySpatialWayfire *instance;

  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, g_object_new (UNITY_SPATIAL_TYPE_WAYFIRE, NULL));

  return instance;
}

UnitySpatialWindowView *
unity_spatial_wayfire_get_window_view (UnitySpatialWayfire *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self), NULL);

  return self->windows;
}

UnitySpatialWorkspaceView *
unity_spatial_wayfire_get_workspace_view (UnitySpatialWayfire *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self), NULL);

  return self->workspaces;
}

void
unity_spatial_wayfire_refresh (UnitySpatialWayfire *self)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self));

  self->stale = TRUE;
  if (self->refreshing)
    return;

  self->refreshing = TRUE;
  dex_future_disown (dex_scheduler_spawn (NULL, 0, refresh_fiber, g_object_ref (self), g_object_unref));
}

void
unity_spatial_wayfire_focus_view (UnitySpatialWayfire *self,
                                  guint                view_id)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self));

  send_view_request (self, "window-rules/focus-view", view_id);
}

void
unity_spatial_wayfire_close_view (UnitySpatialWayfire *self,
                                  guint                view_id)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self));

  send_view_request (self, "window-rules/close-view", view_id);
}

void
unity_spatial_wayfire_send_view (UnitySpatialWayfire *self,
                                 guint                view_id,
                                 gint                 x,
                                 gint                 y)
{
  g_autoptr (JsonObject) data = json_object_new ();

  g_return_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self));

  json_object_set_int_member (data, "view-id", view_id);
  json_object_set_int_member (data, "x", x);
  json_object_set_int_member (data, "y", y);
  unity_spatial_ipc_send (self->ipc, "vswitch/send-view", data);
}

void
unity_spatial_wayfire_set_workspace (UnitySpatialWayfire *self,
                                     gint                 x,
                                     gint                 y)
{
  g_autoptr (JsonObject) data = json_object_new ();

  g_return_if_fail (UNITY_SPATIAL_IS_WAYFIRE (self));

  json_object_set_int_member (data, "output-id", self->output_id);
  json_object_set_int_member (data, "x", x);
  json_object_set_int_member (data, "y", y);
  unity_spatial_ipc_send (self->ipc, "unity-spatial-preview/set-workspace", data);
}
