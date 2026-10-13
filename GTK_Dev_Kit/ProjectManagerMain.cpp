/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProjectManagerMain.cpp - Part of the SLUDGE Project Manager (GTK+ version)
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

#include "ProjectManagerMain.h"
#include "SludgeProjectManager.h"
#include "Common.h"

SludgeProjectManager *projectManager;

#ifdef __cplusplus
extern "C" {
#endif

// SludgeApplication callbacks:

G_MODULE_EXPORT gboolean on_window1_close_request(GtkWindow *theWindow, gpointer theUser_data)
{
	return projectManager->on_window1_delete_event();
}

// SludgeProjectManager callbacks:

G_MODULE_EXPORT void on_files_treeview_realize(GtkTreeView *theTreeView, gpointer theUser_data)
{
	projectManager->on_treeview_realize(theTreeView, FILE_TREEVIEW);
}

G_MODULE_EXPORT void on_resources_treeview_realize(GtkTreeView *theTreeView, gpointer theUser_data)
{
	projectManager->on_treeview_realize(theTreeView, RESOURCE_TREEVIEW);
}

G_MODULE_EXPORT void on_errors_treeview_realize(GtkTreeView *theTreeView, gpointer theUser_data)
{
	projectManager->on_treeview_realize(theTreeView, ERROR_TREEVIEW);
}

G_MODULE_EXPORT void
on_files_tree_selection_changed_cb(GtkTreeSelection *theSelection, gpointer theUser_data)
{
	projectManager->on_files_tree_selection_changed(theSelection);
}

G_MODULE_EXPORT void
on_files_treeview_row_activated(GtkTreeView *theTreeView, GtkTreePath *thePath, GtkTreeViewColumn *theColumn, gpointer theUser_data)
{
	projectManager->on_treeview_row_activated(theTreeView, thePath, theColumn, FILE_TREEVIEW);
}

G_MODULE_EXPORT void
on_resources_treeview_row_activated(GtkTreeView *theTreeView, GtkTreePath *thePath, GtkTreeViewColumn *theColumn, gpointer theUser_data)
{
	projectManager->on_treeview_row_activated(theTreeView, thePath, theColumn, RESOURCE_TREEVIEW);
}

G_MODULE_EXPORT void
on_errors_treeview_row_activated(GtkTreeView *theTreeView, GtkTreePath *thePath, GtkTreeViewColumn *theColumn, gpointer theUser_data)
{
	projectManager->on_treeview_row_activated(theTreeView, thePath, theColumn, ERROR_TREEVIEW);
}

G_MODULE_EXPORT void on_add_file_clicked(GtkButton *theButton, gpointer theUser_data)
{
	projectManager->on_add_file_clicked();
}

G_MODULE_EXPORT void on_remove_file_clicked(GtkButton *theButton, gpointer theUser_data)
{
	projectManager->on_remove_file_clicked();
}

G_MODULE_EXPORT void on_comp_okbutton_clicked(GtkButton *theButton, gpointer theUser_data)
{
	projectManager->on_comp_okbutton_clicked(theButton);
}

G_MODULE_EXPORT void on_comp_gamebutton_clicked(GtkButton *theButton, gpointer theUser_data)
{
	projectManager->on_comp_gamebutton_clicked();
}

G_MODULE_EXPORT void *compile_hook(gpointer nothing)
{
	projectManager->compile();
	return NULL;
}

G_MODULE_EXPORT gboolean update_compile_window_hook(gpointer nothing)
{
	projectManager->update_compile_window();
	return FALSE;
}

void receiveCompilerInfo(compilerInfo *info)
{
	g_async_queue_push(projectManager->compilerInfoQueue, info);
	g_idle_add(update_compile_window_hook, NULL);
}

G_MODULE_EXPORT gboolean on_project_settings_dialog_close_request(GtkWindow *theWindow, gpointer theUser_data)
{
	gtk_widget_set_visible(GTK_WIDGET(theWindow), FALSE);
	return TRUE;
}

G_MODULE_EXPORT gboolean on_preferences_dialog_close_request(GtkWindow *theWindow, gpointer theUser_data)
{
	gtk_widget_set_visible(GTK_WIDGET(theWindow), FALSE);
	return TRUE;
}

G_MODULE_EXPORT gboolean on_compiler_dialog_close_request(GtkWindow *theWindow, gpointer theUser_data)
{
	gtk_widget_set_visible(GTK_WIDGET(theWindow), FALSE);
	return TRUE;
}
#ifdef __cplusplus
}
#endif

// Menu actions:

static void action_new(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_new();
}

static void action_open(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_open();
}

static void action_save(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_save();
}

static void action_save_as(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_save_as();
}

static void action_quit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_window1_delete_event();
}

static void action_preferences(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_preferences();
}

static void action_project_settings(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_project_settings();
}

static void action_compile(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_compile();
}

static void action_comp_gamebutton(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_comp_gamebutton_clicked();
}

static void action_floormaker(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_program_activate(FLOORMAKER);
}

static void action_spritebankeditor(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_program_activate(SPRITEBANKEDITOR);
}

static void action_zbuffermaker(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_program_activate(ZBUFFERMAKER);
}

static void action_translationeditor(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_program_activate(TRANSLATIONEDITOR);
}

static void action_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	projectManager->on_about();
}

static const GActionEntry projectManagerActions[] = {
	{ "new",                 action_new,                 NULL, NULL, NULL },
	{ "open",                action_open,                NULL, NULL, NULL },
	{ "save",                action_save,                NULL, NULL, NULL },
	{ "save-as",             action_save_as,             NULL, NULL, NULL },
	{ "preferences",         action_preferences,         NULL, NULL, NULL },
	{ "project-settings",    action_project_settings,    NULL, NULL, NULL },
	{ "compile",             action_compile,             NULL, NULL, NULL },
	{ "comp-gamebutton",     action_comp_gamebutton,     NULL, NULL, NULL },
	{ "floormaker",          action_floormaker,          NULL, NULL, NULL },
	{ "spritebankeditor",    action_spritebankeditor,    NULL, NULL, NULL },
	{ "zbuffermaker",        action_zbuffermaker,        NULL, NULL, NULL },
	{ "translationeditor",   action_translationeditor,   NULL, NULL, NULL },
	{ "quit",                action_quit,                NULL, NULL, NULL },
	{ "about",               action_about,               NULL, NULL, NULL },
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
		errorBox("Project file not found!", joinTwoStrings("File not found:\n", openThisFile));
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

	projectManager = new SludgeProjectManager();

	/*
	 * Get the window manager to connect any assigned signals in the loaded Glade file to our coded functions.
	 */

	// Make the menu bar actions available to the window.
	GSimpleActionGroup *actionGroup = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(actionGroup), projectManagerActions,
	                                G_N_ELEMENTS(projectManagerActions), NULL);
	gtk_widget_insert_action_group(projectManager->theWindow, "win", G_ACTION_GROUP(actionGroup));
	projectManager->actionGroup = actionGroup;

	// Realize the window first, so that the tree views create their models
	// before a project is loaded into them.
	gtk_window_present(GTK_WINDOW(projectManager->theWindow));

	if (openThisFile != NULL) {
		projectManager->open(openThisFile);
	}

	// Run the window manager loop.
	if (projectManager->initSuccess)
		runMainLoop(projectManager->theWindow);

	delete projectManager;

	return 0;
}
