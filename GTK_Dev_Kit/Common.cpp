/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Common.cpp - Part of the SLUDGE Dev Kit (GTK+ version)
 *
 * Copyright (C) 2010 Tobias Hansen <tobias.han@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "Common.h"
#include "interface.h"

gint message_dialog( GtkMessageType type, const gchar *title, const gchar *message);

static GMainLoop *theMainLoop = NULL;

struct dialogRunData {
	GMainLoop *loop;
	gint response;
};

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
	struct dialogRunData *data = (struct dialogRunData *) user_data;
	data->response = response_id;
	g_main_loop_quit(data->loop);
}

/*
 * GTK 4 removed gtk_dialog_run(). Run a nested main loop until the dialog emits
 * "response", so that the callers can keep working synchronously.
 */
int runDialog(GtkDialog *dialog)
{
	struct dialogRunData data;
	data.response = GTK_RESPONSE_NONE;
	data.loop = g_main_loop_new(NULL, FALSE);
	g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), &data);
	gtk_window_present(GTK_WINDOW(dialog));
	g_main_loop_run(data.loop);
	g_main_loop_unref(data.loop);
	return data.response;
}

/*
 * GTK 4 removed gtk_main() / gtk_main_quit().
 */
void runMainLoop(GtkWidget *window)
{
	if (window != NULL)
		gtk_window_present(GTK_WINDOW(window));

	theMainLoop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(theMainLoop);
	g_main_loop_unref(theMainLoop);
	theMainLoop = NULL;
}

void quitMainLoop(void)
{
	if (theMainLoop != NULL)
		g_main_loop_quit(theMainLoop);
}

char * fileChooserGetFilename(GtkFileChooser *theChooser)
{
	GFile *file = gtk_file_chooser_get_file(theChooser);
	if (file == NULL) return NULL;
	char *path = g_file_get_path(file);
	g_object_unref(file);
	return path;
}

GSList * fileChooserGetFilenames(GtkFileChooser *theChooser)
{
	GSList *list = NULL;
	GListModel *model = gtk_file_chooser_get_files(theChooser);
	if (model != NULL) {
		guint n = g_list_model_get_n_items(model);
		for (guint i = 0; i < n; i++) {
			GFile *file = G_FILE(g_list_model_get_item(model, i));
			char *path = g_file_get_path(file);
			if (path != NULL) list = g_slist_append(list, path);
			g_object_unref(file);
		}
		g_object_unref(model);
	}
	return list;
}

void fileChooserSetCurrentFolder(GtkFileChooser *theChooser, const char *folder)
{
	GFile *file = g_file_new_for_path(folder);
	gtk_file_chooser_set_current_folder(theChooser, file, NULL);
	g_object_unref(file);
}

bool askAQuestion (const char * head, const char * msg)
{
	gint response;

	response = message_dialog(GTK_MESSAGE_QUESTION, head, msg);

	if (response == GTK_RESPONSE_YES) return true;
		else return false;
}

bool errorBox (const char * head, const char * msg)
{
	message_dialog(GTK_MESSAGE_ERROR, head, msg);

	return false;
}

gint
message_dialog(GtkMessageType type, const gchar *title, const gchar *message)
{
	GtkWidget *dialog;
	gint response;
	GtkButtonsType buttons;

	if (type == GTK_MESSAGE_QUESTION) buttons = GTK_BUTTONS_YES_NO;
	else buttons = GTK_BUTTONS_OK;
        
	dialog = gtk_message_dialog_new (NULL,
						GTK_DIALOG_MODAL,
						type,
						buttons,
						"%s", message);
                        
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	response = runDialog (GTK_DIALOG (dialog));
	gtk_window_destroy (GTK_WINDOW (dialog));

	return response;
}

void setFileChooserFilters(GtkFileChooser *theFileChooser, gboolean png, gboolean tga)
{
	GtkFileFilter *pngtgafilter, *pngfilter, *tgafilter;

	if (png && tga) {
		pngtgafilter = gtk_file_filter_new();
		gtk_file_filter_set_name(pngtgafilter, "PNG/TGA images");
		gtk_file_filter_add_mime_type(pngtgafilter, "image/png");
		gtk_file_filter_add_mime_type(pngtgafilter, "image/x-tga");
		gtk_file_filter_add_pattern(pngtgafilter, "*.[tT][gG][aA]");
		gtk_file_chooser_add_filter(theFileChooser, pngtgafilter);
	}
	if (png) {
		pngfilter = gtk_file_filter_new();
		gtk_file_filter_set_name(pngfilter, "PNG images");
		gtk_file_filter_add_mime_type(pngfilter, "image/png");
		gtk_file_chooser_add_filter(theFileChooser, pngfilter);
	}
	if (tga) {
		tgafilter = gtk_file_filter_new();
		gtk_file_filter_set_name(tgafilter, "TGA images");
		gtk_file_filter_add_mime_type(tgafilter, "image/x-tga");
		gtk_file_filter_add_pattern(tgafilter, "*.[tT][gG][aA]");
		gtk_file_chooser_add_filter(theFileChooser, tgafilter);
	}
	if (png && tga) {
		gtk_file_chooser_set_filter(theFileChooser, pngtgafilter);
	} else if (png) {
		gtk_file_chooser_set_filter(theFileChooser, pngfilter);
	} else if (tga) {
		gtk_file_chooser_set_filter(theFileChooser, tgafilter);
	}
}

const char * getTempDir ()
{
	return g_get_user_cache_dir();
}

char * joinTwoStrings (const char * a, const char * b) {
	char * nS = new char[strlen (a) + strlen (b) + 1];
	sprintf (nS, "%s%s", a, b);
	return nS;
}

bool fileExists(char * file) {
	FILE * tester;
	bool retval = false;
	tester = fopen (file, "rb");
	if (tester) {
		retval = true;
		fclose (tester);
	}
	return retval;
}

void flipBackslashes(char **string)
{
#ifdef __WIN32
	for (int i = 0; (*string)[i] != 0; i++)
	{
		if ((*string)[i] == '\\')
			(*string)[i] = '/';
	}
#endif
}

void winChangeToProgramDir(const char *programFullPath)
{
#ifdef __WIN32
	char programPath[1000];
	int lastSlash = 0;
	sprintf(programPath, "%s", programFullPath);
	for (int j = 0; programPath[j] != 0; j++) {
		if (programPath[j] == '\\') {
			programPath[j] = '/';
			lastSlash = j;
		}
	}
	programPath[lastSlash + 1] = NULL;
	g_chdir(programPath);
#endif
}

void replaceInvalidCharacters(char *string, int *retval)
{
	const gchar *end;
	if (!g_utf8_validate(string, -1, &end)) {
		for (int i = 0; string[i] != 0; i++) {
			if (string[i] == end[0])
				string[i] = '_';
		}
		replaceInvalidCharacters(string, retval);
		*retval = 0;
	} else {
		return;
	}
}

int sh_cmd (gchar * path, const gchar * cmd, gchar * args)
{
	char     *quoted_args;
	char     cmd_line[256];
	char   **argv;
	int      argp;
	int      rc = 0;

	if (cmd == NULL)
		return FALSE;

	if (cmd[0] == '\0')
		return FALSE;

	if (path != NULL)
		g_chdir (path);

	if (args == NULL) {
		snprintf (cmd_line, sizeof (cmd_line), "%s", cmd);
	} else {
		#ifdef __WIN32
		for (int i = 0; args[i] != 0; i++)
		{
			if (args[i] == '/')
				args[i] = '\\';
		}
		#endif

		quoted_args = g_shell_quote (args);

		snprintf (cmd_line, sizeof (cmd_line), "%s %s", cmd, quoted_args);

		g_free(quoted_args);
	}

	rc = g_shell_parse_argv (cmd_line, &argp, &argv, NULL);
	if (!rc)
	{
		g_strfreev (argv);
		return rc;
	}

	rc = g_spawn_async (path, argv, NULL,
		      G_SPAWN_SEARCH_PATH,
		      NULL, NULL, NULL, NULL);

	g_strfreev (argv);

	return rc;
}
