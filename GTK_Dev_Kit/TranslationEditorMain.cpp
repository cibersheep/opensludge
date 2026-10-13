/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * TranslationEditorMain.cpp - Part of the SLUDGE TranslationEditor (GTK+ version)
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <glib.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <getopt.h>

#include "TranslationEditorMain.h"
#include "SludgeTranslationEditor.h"
#include "Common.h"

SludgeTranslationEditor *translationEditor;

#ifdef __cplusplus
extern "C" {
#endif

// SludgeApplication callbacks:

G_MODULE_EXPORT gboolean on_window1_close_request(GtkWindow *theWindow, gpointer theUser_data)
{
	return translationEditor->on_window1_delete_event();
}

// SludgeTranslationEditor callbacks:

G_MODULE_EXPORT void on_combobox_realize(GtkComboBox *theComboBox, gpointer theUser_data)
{
	translationEditor->on_combobox_realize(theComboBox);
}

G_MODULE_EXPORT void on_combobox_changed(GtkComboBox *theComboBox, gpointer theUser_data)
{
	translationEditor->on_combobox_changed(theComboBox);
}

G_MODULE_EXPORT void on_treeview_realize(GtkTreeView *theTreeView, gpointer theUser_data)
{
	translationEditor->on_treeview_realize(theTreeView);
}

G_MODULE_EXPORT void
on_tree_selection_changed_cb(GtkTreeSelection *theSelection, gpointer theUser_data)
{
	translationEditor->on_tree_selection_changed(theSelection);
}

G_MODULE_EXPORT void
on_translate_toggled_cb(GtkCellRendererToggle *theCell_renderer, gchar *thePath, gpointer theUser_data)
{
	translationEditor->on_column_changed(COLUMN_TRANSLATE, GTK_CELL_RENDERER(theCell_renderer), thePath, NULL);
}

G_MODULE_EXPORT void
on_translation_edited_cb(GtkCellRendererText *theCell_renderer, gchar *thePath, gchar *theNewText, gpointer theUser_data)
{
	translationEditor->on_column_changed(COLUMN_TRANSLATION, GTK_CELL_RENDERER(theCell_renderer), thePath, theNewText);
}

G_MODULE_EXPORT void
on_sort_original_clicked_cb(GtkTreeViewColumn *theTreeViewColumn, gpointer theUser_data)
{
	translationEditor->on_sort_clicked(theTreeViewColumn, COLUMN_ORIGINAL);
}

G_MODULE_EXPORT void
on_sort_translation_clicked_cb(GtkTreeViewColumn *theTreeViewColumn, gpointer theUser_data)
{
	translationEditor->on_sort_clicked(theTreeViewColumn, COLUMN_TRANSLATION);
}

G_MODULE_EXPORT void on_load_strings_clicked(GtkButton *theButton, gpointer theUser_data)
{
	translationEditor->on_load_strings_clicked();
}

G_MODULE_EXPORT gboolean searchEqualFunc_cb(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
	return translationEditor->searchEqualFunc(model, key, iter);
}

#ifdef __cplusplus
}
#endif

// Menu actions:

static void action_new(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_new();
}

static void action_open(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_open();
}

static void action_save(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_save();
}

static void action_save_as(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_save_as();
}

static void action_load_strings(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_load_strings_clicked();
}

static void action_quit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_window1_delete_event();
}

static void action_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	translationEditor->on_about();
}

static const GActionEntry translationEditorActions[] = {
	{ "new",          action_new,          NULL, NULL, NULL },
	{ "open",         action_open,         NULL, NULL, NULL },
	{ "save",         action_save,         NULL, NULL, NULL },
	{ "save-as",      action_save_as,      NULL, NULL, NULL },
	{ "load-strings", action_load_strings, NULL, NULL, NULL },
	{ "quit",         action_quit,         NULL, NULL, NULL },
	{ "about",        action_about,        NULL, NULL, NULL },
};

void printCmdlineUsage() {
	fprintf(stdout, "SLUDGE Project Manager, usage: sludge-projectmanager <project file>\n");
}

bool parseCmdlineParameters(int argc, char *argv[]) {
	int retval = true;
	while (1)
	{
		static struct option long_options[] =
		{
			{"help",	no_argument,	   0, 'h' },
			{0,0,0,0} /* This is a filler for -1 */
		};
		int option_index = 0;
		int c = getopt_long(argc, argv, "h", long_options, &option_index);
		if (c == -1) break;
			switch (c) {
		case 'h':
		default:
			retval = false;
			break;
		}
	}
	return retval;
}


/*
 * The main function.
 */
int
main(int argc, char *argv[])
{
	char *openThisFile = argv[argc - 1];

	if (! parseCmdlineParameters(argc, argv) ) {
		printCmdlineUsage();
		return 0;
	}

	if (argc < 2) {
		openThisFile = NULL;
	} else if (! fileExists(openThisFile) ) {
		errorBox("Translation file not found!", joinTwoStrings("File not found:\n", openThisFile));
		printCmdlineUsage();
		return -1;
	}

	// On Windows, change to the program directory to
	// make sure the necessary resource files are found:
	winChangeToProgramDir(argv[0]);

	/*
	 * Init GTK+.
	 */
	gtk_init();

	translationEditor = new SludgeTranslationEditor();

	/*
	 * Get the window manager to connect any assigned signals in the loaded Glade file to our coded functions.
	 */

	// Make the menu bar actions available to the window.
	GSimpleActionGroup *actionGroup = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(actionGroup), translationEditorActions,
	                                G_N_ELEMENTS(translationEditorActions), NULL);
	gtk_widget_insert_action_group(translationEditor->theWindow, "win", G_ACTION_GROUP(actionGroup));
	translationEditor->actionGroup = actionGroup;

	// Realize the window first, so that the tree view creates its model
	// before a translation is loaded into it.
	gtk_window_present(GTK_WINDOW(translationEditor->theWindow));

	if (openThisFile != NULL) {
		translationEditor->open(openThisFile);
	}

	// Run the window manager loop.
	if (translationEditor->initSuccess)
		runMainLoop(translationEditor->theWindow);

	delete translationEditor;

	return 0;
}
