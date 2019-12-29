/*

  Copyright 2016-2019 Ataraxia Linux

*/

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "common.h"

#define READ_ERROR 1
#define READ_ERROR_GENERAL 1
#define PIDFILE "/run/timedate1.pid"

char *read_key_file (const char *filename,
                               const char *key)
{
        GIOChannel *channel;
        char       *key_eq;
        char       *line;
        char       *retval;

        if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
                return NULL;

        channel = g_io_channel_new_file (filename, "r", NULL);
        if (!channel)
                return NULL;

        key_eq = g_strdup_printf ("%s=", key);
        retval = NULL;

        while (g_io_channel_read_line (channel, &line, NULL,
                                       NULL, NULL) == G_IO_STATUS_NORMAL) {
                if (g_str_has_prefix (line, key_eq)) {
                        char *value;
                        int   len;

                        value = line + strlen (key_eq);
                        g_strstrip (value);

                        len = strlen (value);

                        if (value[0] == '\"') {
                                if (value[len - 1] == '\"') {
                                        if (retval)
                                                g_free (retval);

                                        retval = g_strndup (value + 1,
                                                            len - 2);
                                }
                        } else {
                                if (retval)
                                        g_free (retval);

                                retval = g_strdup (line + strlen (key_eq));
                        }

                        g_strstrip (retval);
                }

                g_free (line);
        }

        g_free (key_eq);
        g_io_channel_unref (channel);

        return retval;
}

gboolean write_key_file (const char  *filename,
                                const char  *key,
                                const char  *value,
                                GError     **error)
{
        GError    *our_error;
        char      *content;
        gsize      len;
        char      *key_eq;
        char     **lines;
        gboolean   replaced;
        gboolean   retval;
        int        n;
        
        if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
                return TRUE;

        our_error = NULL;

        if (!g_file_get_contents (filename, &content, &len, &our_error)) {
                g_set_error (error, READ_ERROR,
                             READ_ERROR_GENERAL,
                             "%s cannot be read: %s",
                             filename, our_error->message);
                g_error_free (our_error);
                return FALSE;
        }

        lines = g_strsplit (content, "\n", 0);
        g_free (content);

        key_eq = g_strdup_printf ("%s=", key);
        replaced = FALSE;

        for (n = 0; lines[n] != NULL; n++) {
                if (g_str_has_prefix (lines[n], key_eq)) {
                        char     *old_value;
                        gboolean  use_quotes;

                        old_value = lines[n] + strlen (key_eq);
                        g_strstrip (old_value);
                        use_quotes = old_value[0] == '\"';

                        g_free (lines[n]);

                        if (use_quotes)
                                lines[n] = g_strdup_printf ("%s\"%s\"",
                                                            key_eq, value);
                        else
                                lines[n] = g_strdup_printf ("%s%s",
                                                            key_eq, value);

                        replaced = TRUE;
                }
        }

        g_free (key_eq);

        if (!replaced) {
                g_strfreev (lines);
                return TRUE;
        }

        content = g_strjoinv ("\n", lines);
        g_strfreev (lines);

        retval = g_file_set_contents (filename, content, -1, &our_error);
        g_free (content);

        if (!retval) {
                g_set_error (error, READ_ERROR,
                             READ_ERROR_GENERAL,
                             "%s cannot be overwritten: %s",
                             filename, our_error->message);
                g_error_free (our_error);
        }

        return retval;
}

void component_started() {
	gchar *pidstring = NULL;
	GError *err = NULL;
	GFile *pidfile = NULL;

	pidfile = g_file_new_for_path(PIDFILE);
	pidstring = g_strdup_printf ("%lu", (gulong)getpid ());
	if (!g_file_replace_contents (pidfile, pidstring, strlen(pidstring), NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, &err)) {
		g_critical ("Failed to write " PIDFILE ": %s", err->message);
		exit(1);
	}
}
