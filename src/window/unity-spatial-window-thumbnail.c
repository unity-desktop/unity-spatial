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

  GtkButton                 *card;
  GtkImage                  *icon;
  GtkLabel                  *title;
  UnitySpatialPreviewMirror *preview;
  GtkRevealer               *title_revealer;
  GtkRevealer               *close_revealer;
  GtkRevealer               *icon_revealer;
  GtkEventController        *motion;
  UnitySpatialWindowPage    *page;
  gboolean                   chrome;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWindowThumbnail, unity_spatial_window_thumbnail, GTK_TYPE_WIDGET)

typedef enum
{
  PROP_PAGE = 1,
} UnitySpatialWindowThumbnailProperty;

static GParamSpec *properties[PROP_PAGE + 1];

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

static void
update_icon (UnitySpatialWindowThumbnail *self)
{
  const gchar          *app_id = unity_spatial_window_page_get_app_id (self->page);
  AstalAppsApplication *app    = app_id != NULL && *app_id != '\0' ? find_app (app_id) : NULL;
  const gchar          *icon   = app != NULL ? astal_apps_application_get_icon_name (app) : NULL;
  g_autoptr (GIcon)     gicon  = icon != NULL && *icon != '\0' ? g_icon_new_for_string (icon, NULL) : NULL;

  if (gicon != NULL)
    gtk_image_set_from_gicon (self->icon, gicon);
  else
    gtk_image_set_from_icon_name (self->icon, "application-x-executable");
}

static void
update_title (UnitySpatialWindowThumbnail *self)
{
  const gchar *title = unity_spatial_window_page_get_title (self->page);

  gtk_label_set_label (self->title, title);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->card), GTK_ACCESSIBLE_PROPERTY_LABEL, title, -1);
}

static void
update_chrome (UnitySpatialWindowThumbnail *self)
{
  gboolean hover = self->chrome &&
                   gtk_event_controller_motion_contains_pointer (GTK_EVENT_CONTROLLER_MOTION (self->motion));

  gtk_revealer_set_reveal_child (self->title_revealer, hover);
  gtk_revealer_set_reveal_child (self->close_revealer, hover);
  gtk_revealer_set_reveal_child (self->icon_revealer, self->chrome);
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
unity_spatial_window_thumbnail_dispose (GObject *object)
{
  UnitySpatialWindowThumbnail *self = UNITY_SPATIAL_WINDOW_THUMBNAIL (object);
  GtkWidget                   *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_WINDOW_THUMBNAIL);
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))) != NULL)
    gtk_widget_unparent (child);
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
    }
}

static void
unity_spatial_window_thumbnail_class_init (UnitySpatialWindowThumbnailClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = unity_spatial_window_thumbnail_dispose;
  object_class->get_property = unity_spatial_window_thumbnail_get_property;

  properties[PROP_PAGE] =
    g_param_spec_object ("page", NULL, NULL, UNITY_SPATIAL_TYPE_WINDOW_PAGE,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  g_type_ensure (UNITY_SPATIAL_TYPE_PREVIEW_MIRROR);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/unity/spatial/unity-spatial-window-thumbnail.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, card);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, icon);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, title);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, preview);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, title_revealer);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, close_revealer);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, icon_revealer);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWindowThumbnail, motion);
  gtk_widget_class_bind_template_callback (widget_class, close_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, update_chrome);

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
  UnitySpatialWindowThumbnail *self;

  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (page), NULL);

  self       = g_object_new (UNITY_SPATIAL_TYPE_WINDOW_THUMBNAIL, NULL);
  self->page = g_object_ref (page);
  update_icon (self);
  update_title (self);
  bounds_changed_cb (self);
  g_signal_connect_object (page, "notify::app-id", G_CALLBACK (update_icon), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::title", G_CALLBACK (update_title), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::bounds", G_CALLBACK (bounds_changed_cb), self, G_CONNECT_SWAPPED);
  unity_spatial_preview_mirror_set_view_id (self->preview, unity_spatial_window_page_get_view_id (page));
  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->card), "u", unity_spatial_window_page_get_view_id (page));

  return self;
}

UnitySpatialWindowPage *
unity_spatial_window_thumbnail_get_page (UnitySpatialWindowThumbnail *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WINDOW_THUMBNAIL (self), NULL);

  return self->page;
}

void
unity_spatial_window_thumbnail_set_chrome_visible (UnitySpatialWindowThumbnail *self,
                                                   gboolean                     visible)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_THUMBNAIL (self));

  self->chrome = visible;
  update_chrome (self);
}
