/*

  Copyright 2016-2019 Ataraxia Linux

*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dbus/dbus-protocol.h>

#include <glib.h>
#include <gio/gio.h>

#include "common.h"
#include "generated.h"

#define RC_CONF "/etc/rc.conf"
#define LOCALTIME "/etc/localtime"

gchar *timezone_name = NULL;
static MiniTimeDateTimedate1 *timedate1 = NULL;
static guint bus_id = 0;
static gboolean read_only = FALSE;
static GFile *localtime_file = NULL;

static char *
system_timezone_read_conf(void) {
	return read_key_file (RC_CONF,"timezone");
}

static gboolean
system_timezone_write_conf (const char  *tz,
			GError     **error)
{
	return write_key_file (RC_CONF, "timezone", tz, error);
}

static gchar*
get_timezone_name (GError **error) {
	gchar *filebuf = NULL, *filebuf2 = NULL, *ret = NULL;
	gchar *timezone_filename = NULL, *localtime_filename = NULL, *localtime2_filename = NULL;
	GFile *localtime2_file = NULL;

	ret = system_timezone_read_conf();
	if (!ret) {
		g_prefix_error (error, "Unable to read timezone from /etc/rc.conf");
		goto out;
	}

	localtime_filename = g_file_get_path(localtime_file);
	localtime2_filename = g_strdup_printf("/usr/share/zoneinfo/%s", ret);
	localtime2_file = g_file_new_for_path(localtime2_filename);

	if (!g_file_load_contents (localtime_file, NULL, &filebuf, NULL, NULL, error)) {
		g_prefix_error (error, "Unable to read '%s':", localtime_filename);
		goto out;
	}
	if (!g_file_load_contents (localtime2_file, NULL, &filebuf2, NULL, NULL, error)) {
		g_prefix_error (error, "Unable to read '%s':", localtime2_filename);
		goto out;
	}
	if (g_strcmp0 (filebuf, filebuf2))
		g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s and %s differ; %s may be outdated or out of sync with %s", localtime_filename, localtime2_filename, localtime_filename, timezone_filename);

	out:
 		g_free (localtime_filename);
		g_free (localtime2_filename);
		if (localtime_file != NULL)
			g_object_unref (localtime2_file);
		return ret;
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *bus_name,
                 gpointer         user_data)
{
	GError *error = NULL;

	g_debug("Acquired a message bus connection");

	timedate1 = mini_time_date_timedate1_skeleton_new();

	mini_time_date_timedate1_set_timezone (timedate1, timezone_name);
	//mini_time_date_timedate1_set_local_rtc (timedate1, local_rtc);
	//mini_time_date_timedate1_set_ntp (timedate1, use_ntp);

	//g_signal_connect (timedate1, "handle-set-time", G_CALLBACK (on_handle_set_time), NULL);
	//g_signal_connect (timedate1, "handle-set-timezone", G_CALLBACK (on_handle_set_timezone), NULL);
	//g_signal_connect (timedate1, "handle-set-local-rtc", G_CALLBACK (on_handle_set_local_rtc), NULL);
	//g_signal_connect (timedate1, "handle-set-ntp", G_CALLBACK (on_handle_set_ntp), NULL);

	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (timedate1),
						connection,
						"/org/freedesktop/timedate1",
						&error)) {
		if (error != NULL) {
			g_critical ("Failed to export interface on /org/freedesktop/timedate1: %s", error->message);
			exit(1);
		}
	}
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *bus_name,
                  gpointer         user_data)
{
	g_debug ("Acquired the name %s", bus_name);
	component_started();
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *bus_name,
              gpointer         user_data)
{
	if (connection == NULL)
		g_critical ("Failed to acquire a dbus connection");
	else
		g_critical ("Failed to acquire dbus name %s", bus_name);
	exit(1);
}

void destroy(void) {
	g_bus_unown_name(bus_id);
	bus_id = 0;
	read_only = FALSE;
	g_object_unref(localtime_file);
}

void init(gboolean _read_only) {
	GError *error = NULL;
	read_only = _read_only;
	localtime_file = g_file_new_for_path(LOCALTIME);

	//get rtc

	timezone_name = get_timezone_name(&error);
	if (error != NULL) {
		g_warning("%s", error->message);
		g_clear_error(&error);
	}

	bus_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
		"org.freedesktop.timedate1",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		on_bus_acquired,
		on_name_acquired,
		on_name_lost,
		NULL,
		NULL);
}

gint main() {
	GMainLoop *loop = NULL;

	init(read_only);

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	g_main_loop_unref (loop);

	destroy();

	exit (1);
}
