/* unity-spatial-main.c
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <adwaita.h>
#include <libdex.h>

#include "unity-spatial-desktop.h"

static void
color_scheme_changed_cb (GSettings *interface)
{
  g_autofree gchar *scheme = g_settings_get_string (interface, "color-scheme");

  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), g_str_equal (scheme, "prefer-light")
                                                                          ? ADW_COLOR_SCHEME_FORCE_LIGHT
                                                                          : ADW_COLOR_SCHEME_FORCE_DARK);
}

static void
view_visible_cb (UnitySpatialDesktop *desktop,
                 GParamSpec          *pspec,
                 GSimpleAction       *action)
{
  g_simple_action_set_state (action, g_variant_new_boolean (unity_spatial_desktop_get_view_visible (desktop)));
}

static void
startup_cb (GApplication *app)
{
  GSettings *interface = g_settings_new ("org.gnome.desktop.interface");

  g_object_set_data_full (G_OBJECT (app), "interface", interface, g_object_unref);
  g_signal_connect (interface, "changed::color-scheme", G_CALLBACK (color_scheme_changed_cb), NULL);
  color_scheme_changed_cb (interface);
}

static void
activate_cb (GApplication *app)
{
  UnitySpatialDesktop       *desktop;
  g_autoptr (GPropertyAction) page    = NULL;
  g_autoptr (GSimpleAction)   visible = NULL;

  if (g_object_get_data (G_OBJECT (app), "desktop") != NULL)
    return;

  desktop = unity_spatial_desktop_new (GTK_APPLICATION (app));
  g_object_set_data_full (G_OBJECT (app), "desktop", desktop, g_object_unref);

  page    = g_property_action_new ("page", desktop, "page");
  visible = g_simple_action_new_stateful ("view-visible", NULL, g_variant_new_boolean (FALSE));
  g_simple_action_set_enabled (visible, FALSE);
  g_signal_connect_object (desktop, "notify::view-visible", G_CALLBACK (view_visible_cb), visible, G_CONNECT_DEFAULT);
  g_action_map_add_action (G_ACTION_MAP (app), G_ACTION (page));
  g_action_map_add_action (G_ACTION_MAP (app), G_ACTION (visible));

  g_application_hold (app);
}

gint
main (gint   argc,
      gchar *argv[])
{
  g_autoptr (AdwApplication) app = NULL;

  dex_init ();
  if (g_getenv ("WAYLAND_DISPLAY") != NULL)
    g_setenv ("GDK_BACKEND", "wayland", TRUE);

  app = adw_application_new ("org.unity.Spatial", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "startup", G_CALLBACK (startup_cb), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
