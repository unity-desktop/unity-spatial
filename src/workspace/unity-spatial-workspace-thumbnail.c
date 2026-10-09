/* unity-spatial-workspace-thumbnail.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-workspace-thumbnail-private.h"

#include "unity-spatial-preview-mirror.h"
#include "unity-spatial-window-grid-private.h"
#include "unity-spatial-window-view.h"
#include "unity-spatial-workspace-view.h"

struct _UnitySpatialWorkspaceThumbnail
{
  GtkWidget parent_instance;

  UnitySpatialWorkspacePage *workspace;
  UnitySpatialPreviewMirror *backdrop;
  UnitySpatialWindowGrid    *grid;
  GtkButton                 *button;
  gdouble                    progress;
  gdouble                    morph;
  gboolean                   wall;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWorkspaceThumbnail, unity_spatial_workspace_thumbnail, GTK_TYPE_WIDGET)

typedef enum
{
  PROP_WORKSPACE = 1,
  PROP_PROGRESS,
  PROP_MORPH,
  PROP_WALL,
} UnitySpatialWorkspaceThumbnailProperty;

static GParamSpec *properties[PROP_WALL + 1];

static gboolean
shows (UnitySpatialWorkspaceThumbnail *self,
       UnitySpatialWindowPage    *page)
{
  return unity_spatial_window_page_get_workspace_x (page) == unity_spatial_workspace_page_get_x (self->workspace) &&
         unity_spatial_window_page_get_workspace_y (page) == unity_spatial_workspace_page_get_y (self->workspace);
}

static gboolean
filter_page (gpointer item,
             gpointer user_data)
{
  return shows (user_data, item);
}

static void
update (UnitySpatialWorkspaceThumbnail *self)
{
  UnitySpatialWorkspacePage *current    = unity_spatial_workspace_view_get_current (unity_spatial_workspace_view_get_default ());
  gboolean                   is_current = self->workspace == current;
  gdouble                    morph      = is_current ? CLAMP (self->progress, 0, 1) : self->progress >= 1;
  gboolean                   wall       = self->progress > 1;

  gtk_widget_set_can_focus (GTK_WIDGET (self), wall || is_current);
  if (is_current && self->progress == 2)
    gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED, FALSE);
  else
    gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED);

  if (!G_APPROX_VALUE (self->morph, morph, DBL_EPSILON))
    {
      self->morph = morph;
      if (morph > 0)
        gtk_widget_add_css_class (GTK_WIDGET (self), "card");
      else
        gtk_widget_remove_css_class (GTK_WIDGET (self), "card");
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MORPH]);
    }

  if (self->wall != wall)
    {
      self->wall = wall;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WALL]);
    }
}

static gboolean
drop_cb (UnitySpatialWorkspaceThumbnail *self,
         const GValue              *value)
{
  UnitySpatialWindowPage *page = g_value_get_object (value);

  if (shows (self, page))
    return FALSE;

  unity_spatial_window_view_move_page (unity_spatial_window_view_get_default (), page,
                                       unity_spatial_workspace_page_get_x (self->workspace),
                                       unity_spatial_workspace_page_get_y (self->workspace));

  return TRUE;
}

static gboolean
unity_spatial_workspace_thumbnail_grab_focus (GtkWidget *widget)
{
  UnitySpatialWorkspaceThumbnail *self = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (widget);

  if (self->wall)
    return gtk_widget_grab_focus (GTK_WIDGET (self->button));

  return gtk_widget_get_focus_child (GTK_WIDGET (self->grid)) != NULL ||
         gtk_widget_child_focus (GTK_WIDGET (self->grid), GTK_DIR_TAB_FORWARD);
}

static void
unity_spatial_workspace_thumbnail_constructed (GObject *object)
{
  UnitySpatialWorkspaceThumbnail *self    = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (object);
  UnitySpatialWorkspaceView      *view    = unity_spatial_workspace_view_get_default ();
  gint                            x       = unity_spatial_workspace_page_get_x (self->workspace);
  gint                            y       = unity_spatial_workspace_page_get_y (self->workspace);
  g_autoptr (GtkFilterListModel)  windows = NULL;
  g_autofree gchar               *label   = NULL;

  G_OBJECT_CLASS (unity_spatial_workspace_thumbnail_parent_class)->constructed (object);

  windows = gtk_filter_list_model_new (g_object_ref (G_LIST_MODEL (unity_spatial_window_view_get_default ())),
                                       GTK_FILTER (gtk_custom_filter_new (filter_page, self, NULL)));
  unity_spatial_window_grid_set_model (self->grid, G_LIST_MODEL (windows));
  g_object_bind_property (view, "desktop-view-id", self->backdrop, "view-id", G_BINDING_SYNC_CREATE);
  g_signal_connect_object (view, "notify::current", G_CALLBACK (update), self, G_CONNECT_SWAPPED);
  update (self);

  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->button), "(ii)", x, y);
  label = g_strdup_printf ("Workspace %d", y * unity_spatial_workspace_view_get_grid_width (view) + x + 1);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->button), GTK_ACCESSIBLE_PROPERTY_LABEL, label, -1);
}

static void
unity_spatial_workspace_thumbnail_dispose (GObject *object)
{
  UnitySpatialWorkspaceThumbnail *self = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (object);

  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_WORKSPACE_THUMBNAIL);
  g_clear_object (&self->workspace);

  G_OBJECT_CLASS (unity_spatial_workspace_thumbnail_parent_class)->dispose (object);
}

static void
unity_spatial_workspace_thumbnail_get_property (GObject    *object,
                                                guint       prop_id,
                                                GValue     *value,
                                                GParamSpec *pspec)
{
  UnitySpatialWorkspaceThumbnail *self = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (object);

  switch ((UnitySpatialWorkspaceThumbnailProperty) prop_id)
    {
    case PROP_WORKSPACE:
      g_value_set_object (value, self->workspace);
      break;
    case PROP_PROGRESS:
      g_value_set_double (value, self->progress);
      break;
    case PROP_MORPH:
      g_value_set_double (value, self->morph);
      break;
    case PROP_WALL:
      g_value_set_boolean (value, self->wall);
      break;
    }
}

static void
unity_spatial_workspace_thumbnail_set_property (GObject      *object,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec)
{
  UnitySpatialWorkspaceThumbnail *self = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (object);

  switch ((UnitySpatialWorkspaceThumbnailProperty) prop_id)
    {
    case PROP_WORKSPACE:
      self->workspace = g_value_dup_object (value);
      break;
    case PROP_PROGRESS:
      if (G_APPROX_VALUE (self->progress, g_value_get_double (value), DBL_EPSILON))
        break;
      self->progress = g_value_get_double (value);
      update (self);
      g_object_notify_by_pspec (object, pspec);
      break;
    case PROP_MORPH:
    case PROP_WALL:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
unity_spatial_workspace_thumbnail_class_init (UnitySpatialWorkspaceThumbnailClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = unity_spatial_workspace_thumbnail_constructed;
  object_class->dispose      = unity_spatial_workspace_thumbnail_dispose;
  object_class->get_property = unity_spatial_workspace_thumbnail_get_property;
  object_class->set_property = unity_spatial_workspace_thumbnail_set_property;

  widget_class->grab_focus = unity_spatial_workspace_thumbnail_grab_focus;

  /**
   * UnitySpatialWorkspaceThumbnail:workspace:
   *
   * The workspace that the thumbnail shows.
   */
  properties[PROP_WORKSPACE] =
    g_param_spec_object ("workspace", NULL, NULL, UNITY_SPATIAL_TYPE_WORKSPACE_PAGE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWorkspaceThumbnail:progress:
   *
   * The page position of the view, from 0 to 2.
   */
  properties[PROP_PROGRESS] =
    g_param_spec_double ("progress", NULL, NULL, 0, 2, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWorkspaceThumbnail:morph:
   *
   * How far the windows of this workspace are in their grid: the progress for
   * the current workspace, 0 or 1 for the others.
   */
  properties[PROP_MORPH] =
    g_param_spec_double ("morph", NULL, NULL, 0, 1, 1, G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialWorkspaceThumbnail:wall:
   *
   * Whether the view is past the windows page, toward the workspaces page.
   */
  properties[PROP_WALL] =
    g_param_spec_boolean ("wall", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  g_type_ensure (UNITY_SPATIAL_TYPE_PREVIEW_MIRROR);
  g_type_ensure (UNITY_SPATIAL_TYPE_WINDOW_GRID);
  g_type_ensure (UNITY_SPATIAL_TYPE_WINDOW_PAGE);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-workspace-thumbnail.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWorkspaceThumbnail, backdrop);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWorkspaceThumbnail, grid);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWorkspaceThumbnail, button);
  gtk_widget_class_bind_template_callback (widget_class, drop_cb);

  gtk_widget_class_set_css_name (widget_class, "workspacethumbnail");
}

static void
unity_spatial_workspace_thumbnail_init (UnitySpatialWorkspaceThumbnail *self)
{
  self->morph = 1;

  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
unity_spatial_workspace_thumbnail_new (UnitySpatialWorkspacePage *workspace)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_PAGE (workspace), NULL);

  return g_object_new (UNITY_SPATIAL_TYPE_WORKSPACE_THUMBNAIL, "workspace", workspace, NULL);
}

UnitySpatialWorkspacePage *
unity_spatial_workspace_thumbnail_get_workspace (UnitySpatialWorkspaceThumbnail *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_THUMBNAIL (self), NULL);

  return self->workspace;
}
