/* unity-spatial-ipc.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-ipc-private.h"

#include <gio/gunixsocketaddress.h>

#define LENGTH_SIZE 4

struct _UnitySpatialIpc
{
  GObject parent_instance;

  GSocketAddress *address;
  GCancellable   *cancellable;
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
  GSocketAddress *address;
  GBytes         *message;
} Request;

typedef struct
{
  Request       request;
  GWeakRef      owner;
  GCancellable *cancellable;
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
request_init (Request         *request,
              UnitySpatialIpc *self,
              const gchar     *method,
              JsonObject      *data)
{
  request->address = g_object_ref (self->address);
  request->message = encode (method, data);
}

static void
request_clear (Request *request)
{
  g_clear_object (&request->address);
  g_clear_pointer (&request->message, g_bytes_unref);
}

static Request *
request_new (UnitySpatialIpc *self,
             const gchar     *method,
             JsonObject      *data)
{
  Request *request = g_new0 (Request, 1);

  request_init (request, self, method, data);

  return request;
}

static void
request_free (Request *request)
{
  request_clear (request);
  g_free (request);
}

static Watch *
watch_new (UnitySpatialIpc *self,
           JsonObject      *data)
{
  Watch *watch = g_new0 (Watch, 1);

  request_init (&watch->request, self, "window-rules/events/watch", data);
  g_weak_ref_init (&watch->owner, self);
  watch->cancellable = g_object_ref (self->cancellable);

  return watch;
}

static void
watch_free (Watch *watch)
{
  request_clear (&watch->request);
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
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
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

static DexFuture *
call_fiber (gpointer user_data)
{
  Request                      *request    = user_data;
  g_autoptr (GError)            error      = NULL;
  g_autoptr (GSocketConnection) connection = connect_to (request->address, &error);
  JsonNode                     *reply      = NULL;

  if (connection != NULL)
    reply = exchange (connection, request->message, NULL, &error);

  if (reply == NULL)
    return dex_future_new_for_error (g_steal_pointer (&error));

  return dex_future_new_take_boxed (JSON_TYPE_NODE, reply);
}

static void
emit_event (Watch    *watch,
            JsonNode *event)
{
  g_autoptr (UnitySpatialIpc) self = g_weak_ref_get (&watch->owner);
  JsonObject                 *object;

  if (self == NULL || !JSON_NODE_HOLDS_OBJECT (event))
    return;

  object = json_node_get_object (event);
  g_signal_emit (self, signals[SIGNAL_EVENT],
                 g_quark_from_string (json_object_get_string_member_with_default (object, "event", "")), object);
}

static DexFuture *
watch_fiber (gpointer user_data)
{
  Watch                        *watch      = user_data;
  g_autoptr (GError)            error      = NULL;
  g_autoptr (GSocketConnection) connection = connect_to (watch->request.address, &error);
  g_autoptr (JsonNode)          reply      = NULL;

  if (connection != NULL)
    reply = exchange (connection, watch->request.message, watch->cancellable, &error);

  while (reply != NULL)
    {
      g_autoptr (JsonNode) event = read_message (connection, watch->cancellable, &error);

      if (event == NULL)
        break;

      emit_event (watch, event);
    }

  if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    g_warning ("Wayfire IPC events stopped: %s", error->message);

  return dex_future_new_for_error (g_steal_pointer (&error));
}

static DexFuture *
log_failure (DexFuture *future,
             gpointer   user_data)
{
  g_autoptr (GError) error = NULL;

  dex_future_get_value (future, &error);
  g_warning ("Wayfire IPC %s failed: %s", (const gchar *) user_data, error->message);

  return NULL;
}

static void
unity_spatial_ipc_dispose (GObject *object)
{
  UnitySpatialIpc *self = UNITY_SPATIAL_IPC (object);

  g_cancellable_cancel (self->cancellable);
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
    g_signal_new ("event", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, JSON_TYPE_OBJECT);

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
  g_return_val_if_fail (UNITY_SPATIAL_IS_IPC (self), NULL);
  g_return_val_if_fail (method != NULL, NULL);

  if (self->address == NULL)
    return dex_future_new_reject (G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Neither _WAYFIRE_SOCKET nor WAYFIRE_SOCKET is set");

  return dex_scheduler_spawn (NULL, 0, call_fiber, request_new (self, method, data), (GDestroyNotify) request_free);
}

void
unity_spatial_ipc_send (UnitySpatialIpc *self,
                        const gchar     *method,
                        JsonObject      *data)
{
  g_return_if_fail (UNITY_SPATIAL_IS_IPC (self));

  dex_future_disown (dex_future_catch (unity_spatial_ipc_call (self, method, data), log_failure, g_strdup (method),
                                       g_free));
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
