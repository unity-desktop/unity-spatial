/* unity-spatial-window-thumbnail.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-window-thumbnail-private.h"

#include <astal-apps.h>

#include "unity-spatial-preview-mirror.h"
#include "unity-spatial-window-view.h"

struct _UnitySpatialWindowThumbnail
{
  GtkWidget parent_instance;

  GtkButton              *card;
  UnitySpatialWindowPage *page;
  gboolean                chrome;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWindowThumbnail, unity_spatial_window_thumbnail, GTK_TYPE_WIDGET)

typedef enum
{
  PROP_PAGE = 1,
  PROP_CHROME,
} UnitySpatialWindowThumbnailProperty;

static GParamSpec *properties[PROP_CHROME + 1];

static AstalAppsApps *
get_apps (void)
{
  static AstalAppsApps *apps;

  if (g_once_init_enter_pointer (&apps))
    g_once_init_leave_pointer (&apps, g_object_new (ASTAL_APPS_TYPE_APPS,
                                                    "min-score", 50.0,
                                                    "entry-multiplier", 1.0,
                                                    NULL));

  return apps;
}

static AstalAppsApplication *
find_app (const gchar *app_id)
{
  g_autofree gchar     *entry = g_strconcat (app_id, ".desktop", NULL);
  GList                *list  = astal_apps_apps_get_list (get_apps ());
  AstalAppsApplication *found = NULL;
  GList                *fuzzy;

  for (GList *l = list; l != NULL && found == NULL; l = l->next)
    {
      if (g_ascii_strcasecmp (astal_apps_application_get_entry (l->data), entry) == 0 ||
          g_ascii_strcasecmp (astal_apps_application_get_wm_class (l->data) ?: "", app_id) == 0)
        found = l->data;
    }
  g_list_free (list);

  if (found != NULL)
    return found;

  fuzzy = astal_apps_apps_fuzzy_query (get_apps (), app_id);
  found = fuzzy != NULL ? fuzzy->data : NULL;
  g_list_free (fuzzy);

  return found;
}

static GHashTable *
icon_names (void)
{
  static GHashTable *names;

  if (g_once_init_enter_pointer (&names))
    {
      GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

      g_signal_connect_swapped (get_apps (), "notify::list", G_CALLBACK (g_hash_table_remove_all), table);
      g_once_init_leave_pointer (&names, table);
    }

  return names;
}

static GIcon *
icon_for_app_id (UnitySpatialWindowThumbnail *self,
                 const gchar                 *app_id)
{
  const gchar *icon = NULL;
  GIcon       *gicon;

  if (app_id != NULL && *app_id != '\0' && !g_hash_table_lookup_extended (icon_names (), app_id, NULL, (gpointer *) &icon))
    {
      AstalAppsApplication *app = find_app (app_id);

      icon = app != NULL ? astal_apps_application_get_icon_name (app) : NULL;
      g_hash_table_insert (icon_names (), g_strdup (app_id), g_strdup (icon));
    }

  gicon = icon != NULL && *icon != '\0' ? g_icon_new_for_string (icon, NULL) : NULL;

  return gicon != NULL ? gicon : g_themed_icon_new ("application-x-executable");
}

static gboolean
shows_hover_chrome (UnitySpatialWindowThumbnail *self,
                    gboolean                     chrome,
                    gboolean                     contains_pointer)
{
  return chrome && contains_pointer;
}

static void
bounds_changed_cb (UnitySpatialWindowThumbnail *self)
{
  const GdkRectangle *bounds = unity_spatial_window_page_get_bounds (self->page);

  gtk_widget_set_visible (GTK_WIDGET (self), bounds->width > 0 && bounds->height > 0);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
close_clicked_cb (UnitySpatialWindowThumbnail *self)
{
  unity_spatial_window_view_close_page (unity_spatial_window_view_get_default (), self->page);
}

static void
unity_spatial_window_thumbnail_constructed (GObject *object)
{
  UnitySpatialWindowThumbnail *self = UNITY_SPATIAL_WINDOW_THUMBNAIL (object);

  G_OBJECT_CLASS (unity_spatial_window_thumbnail_parent_class)->constructed (object);

  g_signal_connect_object (self->page, "notify::bounds", G_CALLBACK (bounds_changed_cb), self, G_CONNECT_SWAPPED);
  bounds_changed_cb (self);
  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->card), "u", unity_spatial_window_page_get_view_id (self->page));
}

static void
unity_spatial_window_thumbnail_dispose (GObject *object)
{
  UnitySpatialWindowThumbnail *self = UNITY_SPATIAL_WINDOW_THUMBNAIL (object);

  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_WINDOW_THUMBNAIL);
  g_clear_object (&self->page);

  G_OBJECT_CLASS (unity_spatial_window_thumbnail_parent_class)->dispose (object);
}

static void
unity_spatial_window_thumbnail_get_property (GObject    *object,
                                             guint       prop_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  UnitySpatialWindowThumbnail *self = UNITY_SPATIAL_WINDOW_THUMBNAIL (object);

  switch ((UnitySpatialWindowThumbnailProperty) prop_id)
    {
    case PROP_PAGE:
      g_value_set_object (value, self->page);
      break;
    case PROP_CHROME:
      g_value_set_boolean (value, self->chrome);
      break;
    }
}

static void
unity_spatial_window_thumbnail_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  UnitySpatialWindowThumbnail *self = UNITY_SPATIAL_WINDOW_THUMBNAIL (object);

  switch ((UnitySpatialWindowThumbnailProperty) prop_id)
    {
    case PROP_PAGE:
      self->page = g_value_dup_object (value);
      break;
    case PROP_CHROME:
      if (self->chrome == g_value_get_boolean (value))
        break;
      self->chrome = g_value_get_boolean (value);
      g_object_notify_by_pspec (object, pspec);
      break;
    }
}

static void
unity_spatial_window_thumbnail_class_init (UnitySpatialWindowThumbnailClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = unity_spatial_window_thumbnail_constructed;
  object_class->dispose      = unity_spatial_window_thumbnail_dispose;
  object_class->get_property = unity_spatial_window_thumbnail_get_property;
  object_class->set_property = unity_spatial_window_thumbnail_set_property;

  /**
   * UnitySpatialWindowThumbnail:page:
   *
   * The window that the thumbnail shows.
   */
  properties[PROP_PAGE] =
    g_param_spec_object ("page", NULL, NULL, UNITY_SPATIAL_TYPE_WINDOW_PAGE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWindowThumbnail:chrome:
   *
   * Whether the app icon shows, and the title and close button show on hover.
   */
  properties[PROP_CHROME] =
    g_param_spec_boolean ("chrome", NULL, NULL, FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  g_type_ensure (UNITY_SPATIAL_TYPE_PREVIEW_MIRROR);
  g_type_ensure (UNITY_SPATIAL_TYPE_WINDOW_PAGE);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/unity/spatial/unity-spatial-window-thumbnail.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, card);
  gtk_widget_class_bind_template_child_full (widget_class, "title_revealer", FALSE, 0);
  gtk_widget_class_bind_template_child_full (widget_class, "close_revealer", FALSE, 0);
  gtk_widget_class_bind_template_child_full (widget_class, "icon_revealer", FALSE, 0);
  gtk_widget_class_bind_template_callback (widget_class, close_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, icon_for_app_id);
  gtk_widget_class_bind_template_callback (widget_class, shows_hover_chrome);

  gtk_widget_class_set_css_name (widget_class, "windowthumbnail");
}

static void
unity_spatial_window_thumbnail_init (UnitySpatialWindowThumbnail *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

UnitySpatialWindowThumbnail *
unity_spatial_window_thumbnail_new (UnitySpatialWindowPage *page)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (page), NULL);

  return g_object_new (UNITY_SPATIAL_TYPE_WINDOW_THUMBNAIL, "page", page, NULL);
}

UnitySpatialWindowPage *
unity_spatial_window_thumbnail_get_page (UnitySpatialWindowThumbnail *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_THUMBNAIL (self), NULL);

  return self->page;
}
