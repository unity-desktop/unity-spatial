/* unity-spatial-globals.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "unity-spatial-globals-private.h"

#include <astal-wl.h>
#include <gdk/wayland/gdkwayland.h>

static AstalWlGlobal *
find (const struct wl_interface *interface)
{
  g_autoptr (GList) globals = astal_wl_registry_find_globals (astal_wl_registry_get_default (), interface->name);

  if (globals == NULL)
    g_debug ("globals: the compositor has no %s", interface->name);

  return globals != NULL ? globals->data : NULL;
}

static gpointer
bind (const struct wl_interface *interface,
      AstalWlGlobal             *global,
      guint32                    version)
{
  return wl_registry_bind (astal_wl_registry_get_registry (astal_wl_registry_get_default ()), global->name, interface,
                           version);
}

static gpointer
bind_stable (const struct wl_interface *interface,
             guint32                    version)
{
  AstalWlGlobal *global = find (interface);

  return global != NULL ? bind (interface, global, MIN (version, global->version)) : NULL;
}

static gpointer
bind_unstable (const struct wl_interface *interface)
{
  AstalWlGlobal *global = find (interface);

  if (global == NULL)
    return NULL;

  if ((gint) global->version != interface->version)
    {
      g_warning ("globals: %s is version %u, this build needs %d; the compositor plugin and the shell differ",
                 interface->name, global->version, interface->version);
      return NULL;
    }

  return bind (interface, global, global->version);
}

const UnitySpatialGlobals *
unity_spatial_globals_get (void)
{
  static gsize               bound;
  static UnitySpatialGlobals globals;

  if (g_once_init_enter (&bound))
    {
      globals.compositor    = gdk_wayland_display_get_wl_compositor (gdk_display_get_default ());
      globals.empty_region  = wl_compositor_create_region (globals.compositor);
      globals.subcompositor = bind_stable (&wl_subcompositor_interface, 1);
      globals.viewporter    = bind_stable (&wp_viewporter_interface, 1);
      globals.single_pixel  = bind_stable (&wp_single_pixel_buffer_manager_v1_interface, 1);
      globals.previews      = bind_unstable (&zunity_preview_manager_interface);
      globals.gestures      = bind_unstable (&zunity_spatial_gestures_interface);
      if (globals.single_pixel != NULL)
        globals.clear_buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer (globals.single_pixel,
                                                                                          0, 0, 0, 0);
      g_once_init_leave (&bound, 1);
    }

  return &globals;
}
