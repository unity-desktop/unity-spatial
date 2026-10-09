/* unity-spatial-view.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-view-private.h"

#include <math.h>

#include "unity-spatial-enums.h"
#include "unity-spatial-window-view.h"
#include "unity-spatial-workspace-view.h"

struct _UnitySpatialView
{
  GtkWidget parent_instance;

  AdwToolbarView       *toolbar;
  AdwViewStack         *pages;
  GtkSearchEntry       *search;
  UnitySpatialCarousel *carousel;
  AdwAnimation         *animation;
  UnitySpatialPage      page;
  gdouble               progress;
  gboolean              swiping;
  gboolean              enable_search;
};

G_DEFINE_FINAL_TYPE (UnitySpatialView, unity_spatial_view, GTK_TYPE_WIDGET)

typedef enum
{
  PROP_PAGE = 1,
  PROP_PROGRESS,
  PROP_ENABLE_SEARCH,
} UnitySpatialViewProperty;

static GParamSpec *properties[PROP_ENABLE_SEARCH + 1];

typedef enum
{
  SIGNAL_CLOSED,
} UnitySpatialViewSignal;

static guint signals[SIGNAL_CLOSED + 1];

static const guint arrow_keys[] = {
  GDK_KEY_Up,
  GDK_KEY_Down,
  GDK_KEY_Left,
  GDK_KEY_Right,
};

static void
apply_progress (UnitySpatialView *self,
                gdouble           progress)
{
  UnitySpatialWorkspaceThumbnail *current;

  progress = CLAMP (progress, UNITY_SPATIAL_PAGE_DESKTOP, UNITY_SPATIAL_PAGE_WORKSPACES);
  if (G_APPROX_VALUE (self->progress, progress, DBL_EPSILON))
    return;

  self->progress = progress;
  unity_spatial_carousel_set_progress (self->carousel, progress);
  adw_toolbar_view_set_reveal_top_bars (self->toolbar, progress > UNITY_SPATIAL_PAGE_DESKTOP);
  gtk_widget_set_overflow (GTK_WIDGET (self->pages),
                           progress >= UNITY_SPATIAL_PAGE_WINDOWS ? GTK_OVERFLOW_HIDDEN : GTK_OVERFLOW_VISIBLE);

  current = unity_spatial_carousel_get_current (self->carousel);
  if (current != NULL && progress == UNITY_SPATIAL_PAGE_WINDOWS)
    gtk_widget_child_focus (GTK_WIDGET (current), GTK_DIR_TAB_FORWARD);
  else if (current != NULL && progress == UNITY_SPATIAL_PAGE_WORKSPACES)
    gtk_widget_grab_focus (GTK_WIDGET (current));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS]);
}

static gboolean
settled (UnitySpatialView *self)
{
  return !self->swiping && adw_animation_get_state (self->animation) != ADW_ANIMATION_PLAYING;
}

static void
animate_to (UnitySpatialView *self,
            UnitySpatialPage  page,
            gdouble           velocity)
{
  if (self->page != page)
    {
      self->page = page;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAGE]);
    }

  adw_spring_animation_set_value_from (ADW_SPRING_ANIMATION (self->animation), self->progress);
  adw_spring_animation_set_value_to (ADW_SPRING_ANIMATION (self->animation), page);
  adw_spring_animation_set_initial_velocity (ADW_SPRING_ANIMATION (self->animation), velocity);
  adw_animation_play (self->animation);
}

static void
animation_value_cb (gdouble           value,
                    UnitySpatialView *self)
{
  apply_progress (self, value);
}

static void
animation_done_cb (UnitySpatialView *self)
{
  if (self->page == UNITY_SPATIAL_PAGE_DESKTOP)
    g_signal_emit (self, signals[SIGNAL_CLOSED], 0);
}

static void
carousel_page_changed_cb (UnitySpatialView *self)
{
  if (self->page == UNITY_SPATIAL_PAGE_DESKTOP && settled (self))
    g_signal_emit (self, signals[SIGNAL_CLOSED], 0);
}

static void
update_bar_style (UnitySpatialView *self)
{
  gboolean scrolled = FALSE;

  if (g_strcmp0 (adw_view_stack_get_visible_child_name (self->pages), "carousel") == 0)
    g_object_get (self->carousel, "scrolled", &scrolled, NULL);

  adw_toolbar_view_set_top_bar_style (self->toolbar, scrolled ? ADW_TOOLBAR_RAISED : ADW_TOOLBAR_FLAT);
}

static void
search_changed_cb (UnitySpatialView *self)
{
  const gchar *text = gtk_editable_get_text (GTK_EDITABLE (self->search));

  adw_view_stack_set_visible_child_name (self->pages, text[0] != '\0' ? "search" : "carousel");
}

static void
released_cb (UnitySpatialView *self)
{
  unity_spatial_view_set_page (self, UNITY_SPATIAL_PAGE_DESKTOP);
}

static gboolean
arrow_cb (GtkWidget *widget,
          GVariant  *args,
          gpointer   user_data)
{
  return UNITY_SPATIAL_VIEW (widget)->progress <= UNITY_SPATIAL_PAGE_WINDOWS;
}

static void
activate_window_action (GtkWidget   *widget,
                        const gchar *action_name,
                        GVariant    *parameter)
{
  GListModel                *windows = G_LIST_MODEL (unity_spatial_window_view_get_default ());
  UnitySpatialWorkspaceView *view    = unity_spatial_workspace_view_get_default ();
  guint                      view_id = g_variant_get_uint32 (parameter);

  for (guint i = 0; i < g_list_model_get_n_items (windows); i++)
    {
      g_autoptr (UnitySpatialWindowPage) page = g_list_model_get_item (windows, i);
      UnitySpatialWorkspacePage         *workspace;

      if (unity_spatial_window_page_get_view_id (page) != view_id)
        continue;

      workspace = unity_spatial_workspace_view_get_workspace (view, unity_spatial_window_page_get_workspace_x (page),
                                                              unity_spatial_window_page_get_workspace_y (page));
      if (workspace != NULL && workspace != unity_spatial_workspace_view_get_current (view))
        unity_spatial_workspace_view_activate (view, workspace);

      unity_spatial_window_view_activate_page (unity_spatial_window_view_get_default (), page);
      unity_spatial_view_set_page (UNITY_SPATIAL_VIEW (widget), UNITY_SPATIAL_PAGE_DESKTOP);
      return;
    }
}

static void
activate_workspace_action (GtkWidget   *widget,
                           const gchar *action_name,
                           GVariant    *parameter)
{
  UnitySpatialWorkspaceView *view = unity_spatial_workspace_view_get_default ();
  UnitySpatialWorkspacePage *workspace;
  gint                       x;
  gint                       y;

  g_variant_get (parameter, "(ii)", &x, &y);
  workspace = unity_spatial_workspace_view_get_workspace (view, x, y);
  if (workspace == NULL)
    return;

  unity_spatial_workspace_view_activate (view, workspace);
  unity_spatial_view_set_page (UNITY_SPATIAL_VIEW (widget), UNITY_SPATIAL_PAGE_DESKTOP);
}

static void
close_action (GtkWidget   *widget,
              const gchar *action_name,
              GVariant    *parameter)
{
  unity_spatial_view_set_page (UNITY_SPATIAL_VIEW (widget), UNITY_SPATIAL_PAGE_DESKTOP);
}

static void
unity_spatial_view_dispose (GObject *object)
{
  UnitySpatialView *self = UNITY_SPATIAL_VIEW (object);

  g_clear_object (&self->animation);
  gtk_widget_dispose_template (GTK_WIDGET (self), UNITY_SPATIAL_TYPE_VIEW);

  G_OBJECT_CLASS (unity_spatial_view_parent_class)->dispose (object);
}

static void
unity_spatial_view_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  UnitySpatialView *self = UNITY_SPATIAL_VIEW (object);

  switch ((UnitySpatialViewProperty) prop_id)
    {
    case PROP_PAGE:
      g_value_set_enum (value, self->page);
      break;
    case PROP_PROGRESS:
      g_value_set_double (value, self->progress);
      break;
    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, self->enable_search);
      break;
    }
}

static void
unity_spatial_view_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  UnitySpatialView *self = UNITY_SPATIAL_VIEW (object);

  switch ((UnitySpatialViewProperty) prop_id)
    {
    case PROP_PAGE:
      unity_spatial_view_set_page (self, g_value_get_enum (value));
      break;
    case PROP_PROGRESS:
      unity_spatial_view_set_progress (self, g_value_get_double (value));
      break;
    case PROP_ENABLE_SEARCH:
      unity_spatial_view_set_enable_search (self, g_value_get_boolean (value));
      break;
    }
}

static void
unity_spatial_view_class_init (UnitySpatialViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = unity_spatial_view_dispose;
  object_class->get_property = unity_spatial_view_get_property;
  object_class->set_property = unity_spatial_view_set_property;

  /**
   * UnitySpatialView:page:
   *
   * The page that shows or that the view animates to.
   */
  properties[PROP_PAGE] =
    g_param_spec_enum ("page", NULL, NULL, UNITY_TYPE_SPATIAL_PAGE, UNITY_SPATIAL_PAGE_DESKTOP,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialView:progress:
   *
   * The position between the pages: 0 is the desktop, 1 the windows and 2 the
   * workspaces.
   */
  properties[PROP_PROGRESS] =
    g_param_spec_double ("progress", NULL, NULL, UNITY_SPATIAL_PAGE_DESKTOP, UNITY_SPATIAL_PAGE_WORKSPACES,
                         UNITY_SPATIAL_PAGE_DESKTOP, G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * UnitySpatialView:enable-search:
   *
   * Whether the search bar and the search page are available.
   */
  properties[PROP_ENABLE_SEARCH] =
    g_param_spec_boolean ("enable-search", NULL, NULL, TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  /**
   * UnitySpatialView::closed:
   * @self: a #UnitySpatialView
   *
   * Emitted when the view is at the desktop page and stops moving.
   */
  signals[SIGNAL_CLOSED] =
    g_signal_new ("closed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  for (gsize i = 0; i < G_N_ELEMENTS (arrow_keys); i++)
    {
      gtk_widget_class_add_binding (widget_class, arrow_keys[i], GDK_NO_MODIFIER_MASK, arrow_cb, NULL);
      gtk_widget_class_add_binding (widget_class, arrow_keys[i], GDK_CONTROL_MASK, arrow_cb, NULL);
    }

  gtk_widget_class_install_action (widget_class, "spatialview.activate-window", "u", activate_window_action);
  gtk_widget_class_install_action (widget_class, "spatialview.activate-workspace", "(ii)", activate_workspace_action);
  gtk_widget_class_install_action (widget_class, "spatialview.close", NULL, close_action);

  g_type_ensure (UNITY_SPATIAL_TYPE_CAROUSEL);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/unity/spatial/unity-spatial-view.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialView, toolbar);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialView, pages);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialView, search);
  gtk_widget_class_bind_template_child (widget_class, UnitySpatialView, carousel);
  gtk_widget_class_bind_template_callback (widget_class, carousel_page_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, released_cb);
  gtk_widget_class_bind_template_callback (widget_class, search_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, update_bar_style);

  gtk_widget_class_set_css_name (widget_class, "spatialview");
}

static void
unity_spatial_view_init (UnitySpatialView *self)
{
  self->enable_search = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->animation =
    adw_spring_animation_new (GTK_WIDGET (self), 0, 0, adw_spring_params_new (1, 0.5, 500),
                              adw_callback_animation_target_new ((AdwAnimationTargetFunc) animation_value_cb, self, NULL));
  adw_spring_animation_set_clamp (ADW_SPRING_ANIMATION (self->animation), TRUE);
  g_signal_connect_swapped (self->animation, "done", G_CALLBACK (animation_done_cb), self);
}

GtkWidget *
unity_spatial_view_new (void)
{
  return g_object_new (UNITY_SPATIAL_TYPE_VIEW, NULL);
}

UnitySpatialPage
unity_spatial_view_get_page (UnitySpatialView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_VIEW (self), UNITY_SPATIAL_PAGE_DESKTOP);

  return self->page;
}

void
unity_spatial_view_set_page (UnitySpatialView *self,
                             UnitySpatialPage  page)
{
  g_return_if_fail (UNITY_SPATIAL_IS_VIEW (self));

  self->swiping = FALSE;
  animate_to (self, page, 0);
}

gdouble
unity_spatial_view_get_progress (UnitySpatialView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_VIEW (self), 0);

  return self->progress;
}

void
unity_spatial_view_set_progress (UnitySpatialView *self,
                                 gdouble           progress)
{
  g_return_if_fail (UNITY_SPATIAL_IS_VIEW (self));

  adw_animation_pause (self->animation);
  apply_progress (self, progress);
}

gboolean
unity_spatial_view_get_enable_search (UnitySpatialView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_VIEW (self), FALSE);

  return self->enable_search;
}

void
unity_spatial_view_set_enable_search (UnitySpatialView *self,
                                      gboolean          enable_search)
{
  g_return_if_fail (UNITY_SPATIAL_IS_VIEW (self));

  enable_search = !!enable_search;
  if (self->enable_search == enable_search)
    return;

  self->enable_search = enable_search;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENABLE_SEARCH]);
}

UnitySpatialCarousel *
unity_spatial_view_get_carousel (UnitySpatialView *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_VIEW (self), NULL);

  return self->carousel;
}

void
unity_spatial_view_begin_swipe (UnitySpatialView *self)
{
  g_return_if_fail (UNITY_SPATIAL_IS_VIEW (self));

  adw_animation_pause (self->animation);
  self->swiping = TRUE;
}

void
unity_spatial_view_update_swipe (UnitySpatialView *self,
                                 gdouble           progress)
{
  g_return_if_fail (UNITY_SPATIAL_IS_VIEW (self));

  apply_progress (self, progress);
}

void
unity_spatial_view_end_swipe (UnitySpatialView *self,
                              gdouble           velocity,
                              gdouble           to)
{
  g_return_if_fail (UNITY_SPATIAL_IS_VIEW (self));

  self->swiping = FALSE;
  animate_to (self, (UnitySpatialPage) CLAMP (round (to), UNITY_SPATIAL_PAGE_DESKTOP, UNITY_SPATIAL_PAGE_WORKSPACES),
              velocity);
}
