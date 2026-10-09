/* unity-spatial-window-view.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-window-view-private.h"

#include "unity-spatial-wayfire-private.h"

struct _UnitySpatialWindowView
{
  GObject parent_instance;

  GPtrArray *pages;
};

static void unity_spatial_window_view_list_model_init (GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (UnitySpatialWindowView, unity_spatial_window_view, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, unity_spatial_window_view_list_model_init))

static GType
unity_spatial_window_view_get_item_type (GListModel *model)
{
  return UNITY_SPATIAL_TYPE_WINDOW_PAGE;
}

static guint
unity_spatial_window_view_get_n_items (GListModel *model)
{
  return UNITY_SPATIAL_WINDOW_VIEW (model)->pages->len;
}

static gpointer
unity_spatial_window_view_get_item (GListModel *model,
                                    guint       position)
{
  UnitySpatialWindowView *self = UNITY_SPATIAL_WINDOW_VIEW (model);

  if (position >= self->pages->len)
    return NULL;

  return g_object_ref (g_ptr_array_index (self->pages, position));
}

static void
unity_spatial_window_view_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = unity_spatial_window_view_get_item_type;
  iface->get_n_items   = unity_spatial_window_view_get_n_items;
  iface->get_item      = unity_spatial_window_view_get_item;
}

static gboolean
moves (UnitySpatialWindowPage        *page,
       const UnitySpatialWindowState *state)
{
  return unity_spatial_window_page_get_workspace_x (page) != state->workspace_x ||
         unity_spatial_window_page_get_workspace_y (page) != state->workspace_y;
}

void
unity_spatial_window_view_update (UnitySpatialWindowView *self,
                                  GArray                 *windows)
{
  g_autoptr (GPtrArray)  pages   = g_ptr_array_new_full (windows->len, g_object_unref);
  g_autoptr (GHashTable) known   = g_hash_table_new (NULL, NULL);
  gboolean               changed = windows->len != self->pages->len;
  guint                  removed = self->pages->len;

  for (guint i = 0; i < self->pages->len; i++)
    g_hash_table_insert (known, GUINT_TO_POINTER (unity_spatial_window_page_get_view_id (g_ptr_array_index (self->pages, i))),
                         g_ptr_array_index (self->pages, i));

  for (guint i = 0; i < windows->len; i++)
    {
      const UnitySpatialWindowState *state = &g_array_index (windows, UnitySpatialWindowState, i);
      UnitySpatialWindowPage        *page  = g_hash_table_lookup (known, GUINT_TO_POINTER (state->view_id));

      if (page == NULL)
        page = unity_spatial_window_page_new (state->view_id);
      else
        g_object_ref (page);

      changed |= i >= self->pages->len || g_ptr_array_index (self->pages, i) != page || moves (page, state);
      unity_spatial_window_page_update (page, state);
      g_ptr_array_add (pages, page);
    }

  if (changed)
    {
      g_ptr_array_unref (self->pages);
      self->pages = g_steal_pointer (&pages);
      g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, self->pages->len);
    }
}

static void
unity_spatial_window_view_dispose (GObject *object)
{
  UnitySpatialWindowView *self = UNITY_SPATIAL_WINDOW_VIEW (object);

  g_clear_pointer (&self->pages, g_ptr_array_unref);

  G_OBJECT_CLASS (unity_spatial_window_view_parent_class)->dispose (object);
}

static void
unity_spatial_window_view_class_init (UnitySpatialWindowViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = unity_spatial_window_view_dispose;
}

static void
unity_spatial_window_view_init (UnitySpatialWindowView *self)
{
  self->pages = g_ptr_array_new_with_free_func (g_object_unref);
}

UnitySpatialWindowView *
unity_spatial_window_view_get_default (void)
{
  return unity_spatial_wayfire_get_window_view (unity_spatial_wayfire_get_default ());
}

void
unity_spatial_window_view_activate_page (UnitySpatialWindowView *self,
                                         UnitySpatialWindowPage *page)
{
  guint position;

  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_VIEW (self));
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (page));

  unity_spatial_wayfire_focus_view (unity_spatial_wayfire_get_default (), unity_spatial_window_page_get_view_id (page));

  if (g_ptr_array_find (self->pages, page, &position) && position > 0)
    {
      g_ptr_array_insert (self->pages, 0, g_ptr_array_steal_index (self->pages, position));
      g_list_model_items_changed (G_LIST_MODEL (self), 0, position + 1, position + 1);
    }
}

void
unity_spatial_window_view_close_page (UnitySpatialWindowView *self,
                                      UnitySpatialWindowPage *page)
{
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_VIEW (self));
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (page));

  unity_spatial_wayfire_close_view (unity_spatial_wayfire_get_default (), unity_spatial_window_page_get_view_id (page));
}

void
unity_spatial_window_view_move_page (UnitySpatialWindowView *self,
                                     UnitySpatialWindowPage *page,
                                     gint                    workspace_x,
                                     gint                    workspace_y)
{
  guint position;

  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_VIEW (self));
  g_return_if_fail (UNITY_SPATIAL_IS_WINDOW_PAGE (page));

  unity_spatial_wayfire_send_view (unity_spatial_wayfire_get_default (), unity_spatial_window_page_get_view_id (page),
                                   workspace_x, workspace_y);
  unity_spatial_window_page_set_workspace (page, workspace_x, workspace_y);

  if (g_ptr_array_find (self->pages, page, &position))
    g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 1);
}
