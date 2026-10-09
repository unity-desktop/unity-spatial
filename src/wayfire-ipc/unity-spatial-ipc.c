/* unity-spatial-ipc.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-ipc-private.h"

#include <gio/gunixsocketaddress.h>

#define LENGTH_SIZE     4
#define RECONNECT_FIRST 1
#define RECONNECT_LAST  30

struct _UnitySpatialIpc
{
  GObject parent_instance;

  GSocketAddress    *address;
  GCancellable      *cancellable;
  GSocketConnection *connection;
  GQueue             calls;
  gboolean           draining;
};

G_DEFINE_FINAL_TYPE (UnitySpatialIpc, unity_spatial_ipc, G_TYPE_OBJECT)

typedef enum
{
  SIGNAL_EVENT,
  N_SIGNALS,
} UnitySpatialIpcSignal;

static guint signals[N_SIGNALS];

typedef struct
{
  GBytes     *message;
  DexPromise *promise;
} Call;

typedef struct
{
  GSocketAddress *address;
  GBytes         *message;
  GWeakRef        owner;
  GCancellable   *cancellable;
} Watch;

static GBytes *
encode (const gchar *method,
        JsonObject  *data)
{
  g_autoptr (JsonNode)   root    = json_node_new (JSON_NODE_OBJECT);
  g_autoptr (JsonObject) message = json_object_new ();
  g_autofree gchar      *body    = NULL;
  GByteArray            *bytes;
  gsize                  size;
  guint32                length;

  json_object_set_string_member (message, "method", method);
  json_object_set_object_member (message, "data", data != NULL ? json_object_ref (data) : json_object_new ());
  json_node_set_object (root, message);

  body   = json_to_string (root, FALSE);
  size   = strlen (body);
  length = GUINT32_TO_LE ((guint32) size);
  bytes  = g_byte_array_sized_new (LENGTH_SIZE + size);
  g_byte_array_append (bytes, (const guint8 *) &length, LENGTH_SIZE);
  g_byte_array_append (bytes, (const guint8 *) body, size);

  return g_byte_array_free_to_bytes (bytes);
}

static void
call_free (Call *call)
{
  g_clear_pointer (&call->message, g_bytes_unref);
  dex_clear (&call->promise);
  g_free (call);
}

static Watch *
watch_new (UnitySpatialIpc *self,
           JsonObject      *data)
{
  Watch *watch = g_new0 (Watch, 1);

  watch->address = g_object_ref (self->address);
  watch->message = encode ("window-rules/events/watch", data);
  g_weak_ref_init (&watch->owner, self);
  watch->cancellable = g_object_ref (self->cancellable);

  return watch;
}

static void
watch_free (Watch *watch)
{
  g_clear_object (&watch->address);
  g_clear_pointer (&watch->message, g_bytes_unref);
  g_weak_ref_clear (&watch->owner);
  g_clear_object (&watch->cancellable);
  g_free (watch);
}

static DexFuture *
until_cancelled (DexFuture    *future,
                 GCancellable *cancellable)
{
  if (cancellable == NULL)
    return future;

  return dex_future_first (future, dex_cancellable_new_from_cancellable (cancellable), NULL);
}

static GBytes *
read_exactly (GInputStream  *stream,
              gsize          size,
              GCancellable  *cancellable,
              GError       **error)
{
  g_autoptr (GByteArray) buffer = g_byte_array_sized_new (size);

  while (buffer->len < size)
    {
      g_autoptr (GBytes) chunk = dex_await_boxed (until_cancelled (dex_input_stream_read_bytes (stream, size - buffer->len,
                                                                                               G_PRIORITY_DEFAULT),
                                                                   cancellable),
                                                  error);
      gconstpointer      data;
      gsize              length;

      if (chunk == NULL)
        return NULL;

      data = g_bytes_get_data (chunk, &length);
      if (length == 0)
        {
          g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED, "Wayfire closed the IPC connection");
          return NULL;
        }

      g_byte_array_append (buffer, data, length);
    }

  return g_byte_array_free_to_bytes (g_steal_pointer (&buffer));
}

static JsonNode *
read_message (GSocketConnection  *connection,
              GCancellable       *cancellable,
              GError            **error)
{
  GInputStream          *stream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  g_autoptr (GBytes)     header = read_exactly (stream, LENGTH_SIZE, cancellable, error);
  g_autoptr (GBytes)     body   = NULL;
  g_autoptr (JsonParser) parser = json_parser_new ();
  guint32                length;
  gconstpointer          data;
  gsize                  size;

  if (header == NULL)
    return NULL;

  memcpy (&length, g_bytes_get_data (header, NULL), LENGTH_SIZE);
  body = read_exactly (stream, GUINT32_FROM_LE (length), cancellable, error);
  if (body == NULL)
    return NULL;

  data = g_bytes_get_data (body, &size);
  if (!json_parser_load_from_data (parser, data, (gssize) size, error))
    return NULL;

  return json_node_copy (json_parser_get_root (parser));
}

static JsonNode *
exchange (GSocketConnection  *connection,
          GBytes             *message,
          GCancellable       *cancellable,
          GError            **error)
{
  GOutputStream *stream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
  JsonNode      *reply;
  JsonObject    *object;

  if (!dex_await (until_cancelled (dex_output_stream_write_bytes (stream, message, G_PRIORITY_DEFAULT), cancellable), error))
    return NULL;

  reply = read_message (connection, cancellable, error);
  if (reply == NULL || !JSON_NODE_HOLDS_OBJECT (reply))
    return reply;

  object = json_node_get_object (reply);
  if (json_object_has_member (object, "error"))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "%s",
                   json_object_get_string_member_with_default (object, "error", ""));
      json_node_unref (reply);
      return NULL;
    }

  return reply;
}

static GSocketConnection *
connect_to (GSocketAddress  *address,
            GError         **error)
{
  g_autoptr (GSocketClient) client = g_socket_client_new ();

  return dex_await_object (dex_socket_client_connect (client, G_SOCKET_CONNECTABLE (address)), error);
}

static JsonNode *
send_call (UnitySpatialIpc  *self,
           GBytes           *message,
           GError          **error)
{
  for (gint attempt = 0; attempt < 2; attempt++)
    {
      JsonNode *reply;

      g_clear_error (error);
      if (self->connection == NULL)
        self->connection = connect_to (self->address, error);
      if (self->connection == NULL)
        return NULL;

      reply = exchange (self->connection, message, self->cancellable, error);
      if (reply != NULL || g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT) ||
          g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return reply;

      g_clear_object (&self->connection);
    }

  return NULL;
}

static DexFuture *
drain_fiber (gpointer user_data)
{
  UnitySpatialIpc *self = user_data;
  Call            *call;

  while ((call = g_queue_pop_head (&self->calls)) != NULL)
    {
      g_autoptr (GError) error = NULL;
      JsonNode          *reply = send_call (self, call->message, &error);

      if (reply != NULL)
        dex_promise_resolve_boxed (call->promise, JSON_TYPE_NODE, reply);
      else
        dex_promise_reject (call->promise, g_steal_pointer (&error));
      call_free (call);
    }

  self->draining = FALSE;

  return dex_future_new_true ();
}

static void
emit_event (Watch      *watch,
            JsonNode   *event)
{
  g_autoptr (UnitySpatialIpc) self = g_weak_ref_get (&watch->owner);
  const gchar                *name = NULL;

  if (event != NULL && JSON_NODE_HOLDS_OBJECT (event))
    name = json_object_get_string_member_with_default (json_node_get_object (event), "event", NULL);

  if (self != NULL)
    g_signal_emit (self, signals[SIGNAL_EVENT], 0, name);
}

static DexFuture *
watch_fiber (gpointer user_data)
{
  Watch *watch = user_data;
  gint   delay = RECONNECT_FIRST;

  while (!g_cancellable_is_cancelled (watch->cancellable))
    {
      g_autoptr (GError)            error      = NULL;
      g_autoptr (GSocketConnection) connection = connect_to (watch->address, &error);
      g_autoptr (JsonNode)          reply      = NULL;

      if (connection != NULL)
        reply = exchange (connection, watch->message, watch->cancellable, &error);

      if (reply != NULL)
        {
          delay = RECONNECT_FIRST;
          emit_event (watch, NULL);
        }

      while (reply != NULL)
        {
          g_autoptr (JsonNode) event = read_message (connection, watch->cancellable, &error);

          if (event == NULL)
            break;

          emit_event (watch, event);
        }

      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        break;

      g_warning ("Wayfire IPC events stopped, retrying in %d s: %s", delay, error->message);
      dex_await (until_cancelled (dex_timeout_new_seconds (delay), watch->cancellable), NULL);
      delay = MIN (delay * 2, RECONNECT_LAST);
    }

  return dex_future_new_true ();
}

static void
unity_spatial_ipc_dispose (GObject *object)
{
  UnitySpatialIpc *self = UNITY_SPATIAL_IPC (object);

  g_cancellable_cancel (self->cancellable);
  g_queue_clear_full (&self->calls, (GDestroyNotify) call_free);
  g_clear_object (&self->connection);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->address);

  G_OBJECT_CLASS (unity_spatial_ipc_parent_class)->dispose (object);
}

static void
unity_spatial_ipc_class_init (UnitySpatialIpcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = unity_spatial_ipc_dispose;

  signals[SIGNAL_EVENT] =
    g_signal_new ("event", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  dex_init ();
}

static void
unity_spatial_ipc_init (UnitySpatialIpc *self)
{
  const gchar *path = g_getenv ("_WAYFIRE_SOCKET");

  if (path == NULL)
    path = g_getenv ("WAYFIRE_SOCKET");

  self->cancellable = g_cancellable_new ();
  if (path != NULL)
    self->address = g_unix_socket_address_new (path);
}

UnitySpatialIpc *
unity_spatial_ipc_new (void)
{
  return g_object_new (UNITY_SPATIAL_TYPE_IPC, NULL);
}

DexFuture *
unity_spatial_ipc_call (UnitySpatialIpc *self,
                        const gchar     *method,
                        JsonObject      *data)
{
  Call *call;

  g_return_val_if_fail (UNITY_SPATIAL_IS_IPC (self), NULL);
  g_return_val_if_fail (method != NULL, NULL);

  if (self->address == NULL)
    return dex_future_new_reject (G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Neither _WAYFIRE_SOCKET nor WAYFIRE_SOCKET is set");

  call          = g_new0 (Call, 1);
  call->message = encode (method, data);
  call->promise = dex_promise_new ();
  g_queue_push_tail (&self->calls, call);

  if (!self->draining)
    {
      self->draining = TRUE;
      dex_future_disown (dex_scheduler_spawn (NULL, 0, drain_fiber, g_object_ref (self), g_object_unref));
    }

  return dex_ref (call->promise);
}

void
unity_spatial_ipc_watch (UnitySpatialIpc     *self,
                         const gchar * const *events)
{
  g_autoptr (JsonObject) data = json_object_new ();
  JsonArray             *list = json_array_new ();

  g_return_if_fail (UNITY_SPATIAL_IS_IPC (self));
  g_return_if_fail (events != NULL);

  if (self->address == NULL)
    {
      g_warning ("Cannot watch Wayfire IPC events: neither _WAYFIRE_SOCKET nor WAYFIRE_SOCKET is set");
      return;
    }

  for (gsize i = 0; events[i] != NULL; i++)
    json_array_add_string_element (list, events[i]);

  json_object_set_array_member (data, "events", list);

  dex_future_disown (dex_scheduler_spawn (NULL, 0, watch_fiber, watch_new (self, data), (GDestroyNotify) watch_free));
}
