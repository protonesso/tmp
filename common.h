/*

  Copyright 2016-2019 Ataraxia Linux

*/

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

char *read_key_file (const char *filename,
                               const char *key);
gboolean
write_key_file (const char  *filename,
                                const char  *key,
                                const char  *value,
                                GError     **error);
void component_started();
