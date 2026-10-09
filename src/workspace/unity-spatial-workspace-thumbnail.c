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
  gdouble                    morph;
  gboolean                   wall;
};

G_DEFINE_FINAL_TYPE (UnitySpatialWorkspaceThumbnail, unity_spatial_workspace_thumbnail, GTK_TYPE_WIDGET)

typedef enum
{
  SIGNAL_ACTIVATE,
} UnitySpatialWorkspaceThumbnailSignal;

static guint signals[SIGNAL_ACTIVATE + 1];

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
released_cb (UnitySpatialWorkspaceThumbnail *self,
             gint                       n_press,
             gdouble                    x,
             gdouble                    y,
             GtkGestureClick           *click)
{
  if (!self->wall)
    return;

  gtk_gesture_set_state (GTK_GESTURE (click), GTK_EVENT_SEQUENCE_CLAIMED);
  gtk_widget_activate (GTK_WIDGET (self));
}

static void
activate_cb (UnitySpatialWorkspaceThumbnail *self)
{
  gtk_widget_activate_action (GTK_WIDGET (self), "spatialview.activate-workspace", "(ii)",
                              unity_spatial_workspace_page_get_x (self->workspace),
                              unity_spatial_workspace_page_get_y (self->workspace));
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

static void
unity_spatial_workspace_thumbnail_dispose (GObject *object)
{
  UnitySpatialWorkspaceThumbnail *self = UNITY_SPATIAL_WORKSPACE_THUMBNAIL (object);

  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_WORKSPACE_THUMBNAIL);
  g_clear_object (&self->workspace);

  G_OBJECT_CLASS (unity_spatial_workspace_thumbnail_parent_class)->dispose (object);
}

static void
unity_spatial_workspace_thumbnail_class_init (UnitySpatialWorkspaceThumbnailClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_spatial_workspace_thumbnail_dispose;

  signals[SIGNAL_ACTIVATE] =
    g_signal_new_class_handler ("activate", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (activate_cb), NULL, NULL, NULL, G_TYPE_NONE, 0);
  gtk_widget_class_set_activate_signal (widget_class, signals[SIGNAL_ACTIVATE]);

  g_type_ensure (UNITY_SPATIAL_TYPE_PREVIEW_MIRROR);
  g_type_ensure (UNITY_SPATIAL_TYPE_WINDOW_GRID);
  g_type_ensure (UNITY_SPATIAL_TYPE_WINDOW_PAGE);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-workspace-thumbnail.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWorkspaceThumbnail, backdrop);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialWorkspaceThumbnail, grid);
  gtk_widget_class_bind_template_callback (widget_class, drop_cb);
  gtk_widget_class_bind_template_callback (widget_class, released_cb);

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
  UnitySpatialWorkspaceView     *view    = unity_spatial_workspace_view_get_default ();
  UnitySpatialWorkspaceThumbnail     *self;
  g_autoptr (GtkFilterListModel) windows = NULL;
  g_autofree gchar              *label   = NULL;

  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_PAGE (workspace), NULL);

  self            = g_object_new (UNITY_SPATIAL_TYPE_WORKSPACE_THUMBNAIL, NULL);
  self->workspace = g_object_ref (workspace);

  windows = gtk_filter_list_model_new (g_object_ref (G_LIST_MODEL (unity_spatial_window_view_get_default ())),
                                       GTK_FILTER (gtk_custom_filter_new (filter_page, self, NULL)));
  unity_spatial_window_grid_set_model (self->grid, G_LIST_MODEL (windows));
  g_object_bind_property (view, "desktop-view-id", self->backdrop, "view-id", G_BINDING_SYNC_CREATE);

  label = g_strdup_printf ("Workspace %d", unity_spatial_workspace_page_get_y (workspace) *
                                           unity_spatial_workspace_view_get_grid_width (view) +
                                           unity_spatial_workspace_page_get_x (workspace) + 1);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, label, -1);

  return GTK_WIDGET (self);
}

UnitySpatialWorkspacePage *
unity_spatial_workspace_thumbnail_get_workspace (UnitySpatialWorkspaceThumbnail *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_WORKSPACE_THUMBNAIL (self), NULL);

  return self->workspace;
}

void
unity_spatial_workspace_thumbnail_set_morph (UnitySpatialWorkspaceThumbnail *self,
                                        gdouble                    morph)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACE_THUMBNAIL (self));

  if (G_APPROX_VALUE (self->morph, morph, DBL_EPSILON))
    return;

  self->morph = morph;
  if (morph > 0)
    gtk_widget_add_css_class (GTK_WIDGET (self), "card");
  else
    gtk_widget_remove_css_class (GTK_WIDGET (self), "card");
  unity_spatial_window_grid_set_morph (self->grid, morph);
}

void
unity_spatial_workspace_thumbnail_set_wall (UnitySpatialWorkspaceThumbnail *self,
                                       gboolean                   wall)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WORKSPACE_THUMBNAIL (self));

  if (self->wall == wall)
    return;

  self->wall = wall;
  gtk_widget_set_focusable (GTK_WIDGET (self), wall);
  gtk_widget_set_can_focus (GTK_WIDGET (self->grid), !wall);
  unity_spatial_window_grid_set_wall (self->grid, wall);
}
