/* unity-spatial-preview-mirror.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-preview-mirror-private.h"

#include <gdk/wayland/gdkwayland.h>

#include "unity-spatial-geometry-private.h"
#include "unity-spatial-globals-private.h"
#include "unity-spatial-mirror-stack-private.h"

struct _UnitySpatialPreviewMirror
{
  GtkWidget parent_instance;

  guint                    view_id;
  UnitySpatialMirrorStack *stack;
  struct wl_surface       *surface;
  struct wl_subsurface    *subsurface;
  struct wp_viewport      *viewport;
  struct zunity_preview   *preview;
  GdkRectangle             rect;
  gboolean                 shown;
};

G_DEFINE_FINAL_TYPE (UnitySpatialPreviewMirror, unity_spatial_preview_mirror, GTK_TYPE_WIDGET)

typedef enum
{
  PROP_VIEW_ID = 1,
} UnitySpatialPreviewMirrorProperty;

static GParamSpec *properties[PROP_VIEW_ID + 1];

static void
attach (UnitySpatialPreviewMirror *self)
{
  const UnitySpatialGlobals *globals = unity_spatial_globals_get ();
  GtkNative                 *native  = gtk_widget_get_native (GTK_WIDGET (self));
  struct wl_surface         *parent;

  if (self->surface != NULL || self->view_id == 0 || !gtk_widget_get_mapped (GTK_WIDGET (self)) ||
      globals->previews == NULL || globals->subcompositor == NULL || globals->viewporter == NULL ||
      globals->clear_buffer == NULL)
    return;

  parent           = gdk_wayland_surface_get_wl_surface (gtk_native_get_surface (native));
  self->surface    = wl_compositor_create_surface (globals->compositor);
  self->subsurface = wl_subcompositor_get_subsurface (globals->subcompositor, self->surface, parent);
  self->viewport   = wp_viewporter_get_viewport (globals->viewporter, self->surface);
  self->preview    = zunity_preview_manager_get_preview (globals->previews, self->surface, self->view_id);
  self->rect       = (GdkRectangle) { 0, 0, 0, 0 };
  self->shown      = FALSE;

  wl_surface_set_input_region (self->surface, globals->empty_region);
  wl_subsurface_place_below (self->subsurface, parent);

  self->stack = unity_spatial_mirror_stack_get_for_native (native);
  unity_spatial_mirror_stack_add (self->stack, self);
}

static void
detach (UnitySpatialPreviewMirror *self)
{
  if (self->surface == NULL)
    return;

  unity_spatial_mirror_stack_remove (self->stack, self);
  g_clear_object (&self->stack);
  g_clear_pointer (&self->preview, zunity_preview_destroy);
  g_clear_pointer (&self->viewport, wp_viewport_destroy);
  g_clear_pointer (&self->subsurface, wl_subsurface_destroy);
  g_clear_pointer (&self->surface, wl_surface_destroy);
}

static gboolean
widget_rect (UnitySpatialPreviewMirror *self,
             GdkRectangle              *rect)
{
  GtkNative      *native = gtk_widget_get_native (GTK_WIDGET (self));
  graphene_rect_t bounds;
  gdouble         dx;
  gdouble         dy;

  if (!gtk_widget_is_drawable (GTK_WIDGET (self)) ||
      !gtk_widget_compute_bounds (GTK_WIDGET (self), GTK_WIDGET (native), &bounds) ||
      bounds.size.width < 1 || bounds.size.height < 1)
    return FALSE;

  gtk_native_get_surface_transform (native, &dx, &dy);
  graphene_rect_offset (&bounds, dx, dy);
  *rect = unity_spatial_snap_rect (&bounds);

  return TRUE;
}

gboolean
unity_spatial_preview_mirror_sync (UnitySpatialPreviewMirror *self)
{
  GdkRectangle rect;
  gboolean     shown   = widget_rect (self, &rect);
  gboolean     changed = shown != self->shown;

  if (shown && (rect.x != self->rect.x || rect.y != self->rect.y))
    {
      wl_subsurface_set_position (self->subsurface, rect.x, rect.y);
      changed = TRUE;
    }

  if (shown && (rect.width != self->rect.width || rect.height != self->rect.height))
    {
      wp_viewport_set_destination (self->viewport, rect.width, rect.height);
      changed = TRUE;
    }

  if (shown != self->shown)
    wl_surface_attach (self->surface, shown ? unity_spatial_globals_get ()->clear_buffer : NULL, 0, 0);

  if (shown)
    self->rect = rect;
  self->shown = shown;

  if (changed)
    wl_surface_commit (self->surface);

  return changed;
}

void
unity_spatial_preview_mirror_place_below (UnitySpatialPreviewMirror *self,
                                          struct wl_surface         *parent)
{
  wl_subsurface_place_below (self->subsurface, parent);
}

static void
unity_spatial_preview_mirror_map (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (unity_spatial_preview_mirror_parent_class)->map (widget);

  attach (UNITY_SPATIAL_PREVIEW_MIRROR (widget));
}

static void
unity_spatial_preview_mirror_unmap (GtkWidget *widget)
{
  detach (UNITY_SPATIAL_PREVIEW_MIRROR (widget));

  GTK_WIDGET_CLASS (unity_spatial_preview_mirror_parent_class)->unmap (widget);
}

static void
unity_spatial_preview_mirror_snapshot (GtkWidget   *widget,
                                       GtkSnapshot *snapshot)
{
  gtk_snapshot_append_color (snapshot, &(GdkRGBA) { 0, 0, 0, 0 },
                             &GRAPHENE_RECT_INIT (0, 0, gtk_widget_get_width (widget), gtk_widget_get_height (widget)));
}

static void
unity_spatial_preview_mirror_dispose (GObject *object)
{
  detach (UNITY_SPATIAL_PREVIEW_MIRROR (object));

  G_OBJECT_CLASS (unity_spatial_preview_mirror_parent_class)->dispose (object);
}

static void
unity_spatial_preview_mirror_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  UnitySpatialPreviewMirror *self = UNITY_SPATIAL_PREVIEW_MIRROR (object);

  switch ((UnitySpatialPreviewMirrorProperty) prop_id)
    {
    case PROP_VIEW_ID:
      g_value_set_uint (value, self->view_id);
      break;
    }
}

static void
unity_spatial_preview_mirror_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  UnitySpatialPreviewMirror *self = UNITY_SPATIAL_PREVIEW_MIRROR (object);

  switch ((UnitySpatialPreviewMirrorProperty) prop_id)
    {
    case PROP_VIEW_ID:
      unity_spatial_preview_mirror_set_view_id (self, g_value_get_uint (value));
      break;
    }
}

static void
unity_spatial_preview_mirror_class_init (UnitySpatialPreviewMirrorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = unity_spatial_preview_mirror_dispose;
  object_class->get_property = unity_spatial_preview_mirror_get_property;
  object_class->set_property = unity_spatial_preview_mirror_set_property;

  widget_class->map      = unity_spatial_preview_mirror_map;
  widget_class->unmap    = unity_spatial_preview_mirror_unmap;
  widget_class->snapshot = unity_spatial_preview_mirror_snapshot;

  properties[PROP_VIEW_ID] =
    g_param_spec_uint ("view-id", NULL, NULL, 0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_css_name (widget_class, "preview");
}

static void
unity_spatial_preview_mirror_init (UnitySpatialPreviewMirror *self)
{
}

GtkWidget *
unity_spatial_preview_mirror_new (guint view_id)
{
  return g_object_new (UNITY_SPATIAL_TYPE_PREVIEW_MIRROR, "view-id", view_id, NULL);
}

guint
unity_spatial_preview_mirror_get_view_id (UnitySpatialPreviewMirror *self)
{
  g_return_val_if_fail (UNITY_SPATIAL_IS_PREVIEW_MIRROR (self), 0);

  return self->view_id;
}

void
unity_spatial_preview_mirror_set_view_id (UnitySpatialPreviewMirror *self,
                                          guint                      view_id)
{
  g_return_if_fail (UNITY_SPATIAL_IS_PREVIEW_MIRROR (self));

  if (self->view_id == view_id)
    return;

  detach (self);
  self->view_id = view_id;
  attach (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VIEW_ID]);
}
