/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ZBufferMakerMain.cpp - Part of the SLUDGE Z-Buffer Maker (GTK+ version)
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

#include <epoxy/gl.h>

#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <getopt.h>

#include "SludgeZBufferMaker.h"
#include "Common.h"

SludgeZBufferMaker *zBufferMaker;

#ifdef __cplusplus
extern "C" {
#endif

// SludgeApplication callbacks:

G_MODULE_EXPORT gboolean on_window1_close_request(GtkWindow *theWindow, gpointer theUser_data)
{
	return zBufferMaker->on_window1_delete_event();
}

// SludgeGLApplication callbacks:

G_MODULE_EXPORT void on_drawingarea1_realize(GtkWidget *theWidget, gpointer theUser_data)
{
	zBufferMaker->on_drawingarea1_realize(theWidget);
}

G_MODULE_EXPORT gboolean on_drawingarea1_render(GtkWidget *theWidget, GdkGLContext *context, gpointer theUser_data)
{
	return zBufferMaker->on_drawingarea1_render(theWidget, context);
}

G_MODULE_EXPORT gboolean render_timer_event(gpointer theUser_data)
{
	return zBufferMaker->render_timer_event(theUser_data);
}

G_MODULE_EXPORT void on_zoom_100_clicked(GtkButton *theButton, gpointer theUser_data)
{
	zBufferMaker->on_zoom_100_clicked();
}

G_MODULE_EXPORT void on_zoom_fit_clicked(GtkButton *theButton, gpointer theUser_data)
{
	zBufferMaker->on_zoom_fit_clicked();
}

// SludgeZBufferMaker callbacks:

G_MODULE_EXPORT void on_hscale_value_changed(GtkScale *theScale, gpointer theUser_data)
{
	if (zBufferMaker == NULL) return;
	zBufferMaker->on_hscale_value_changed(theScale);
}

G_MODULE_EXPORT void on_spinbutton_value_changed(GtkSpinButton *theSpinButton, gpointer theUser_data)
{
	if (zBufferMaker == NULL) return;
	zBufferMaker->on_spinbutton_value_changed(theSpinButton);
}

#ifdef __cplusplus
}
#endif

// Menu actions:

static void action_new(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	zBufferMaker->on_new();
}

static void action_open(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	zBufferMaker->on_open();
}

static void action_save(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	zBufferMaker->on_save();
}

static void action_save_as(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	zBufferMaker->on_save_as();
}

static void action_quit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	zBufferMaker->on_window1_delete_event();
}

static void action_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	zBufferMaker->on_about();
}

static const GActionEntry zBufferMakerActions[] = {
	{ "new",     action_new,     NULL, NULL, NULL },
	{ "open",    action_open,    NULL, NULL, NULL },
	{ "save",    action_save,    NULL, NULL, NULL },
	{ "save-as", action_save_as, NULL, NULL, NULL },
	{ "quit",    action_quit,    NULL, NULL, NULL },
	{ "about",   action_about,   NULL, NULL, NULL },
};

void printCmdlineUsage() {
	fprintf(stdout, "SLUDGE Z-Buffer Maker, usage: sludge-zbuffermaker <z-buffer file>\n");
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
		fprintf(stderr, "Z-Buffer file not found.\n");
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

	zBufferMaker = new SludgeZBufferMaker();

	/*
	 * Get the window manager to connect any assigned signals in the loaded Glade file to our coded functions.
	 */

	// Make the menu bar actions available to the window.
	GSimpleActionGroup *actionGroup = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(actionGroup), zBufferMakerActions,
	                                G_N_ELEMENTS(zBufferMakerActions), NULL);
	gtk_widget_insert_action_group(zBufferMaker->theWindow, "win", G_ACTION_GROUP(actionGroup));
	zBufferMaker->actionGroup = actionGroup;

	// Start the render timer.
	g_timeout_add(10000 / 60, render_timer_event, zBufferMaker->theDrawingarea);

	if (openThisFile != NULL) {
		zBufferMaker->open(openThisFile);
	}

	// Run the window manager loop.
	if (zBufferMaker->initSuccess)
		runMainLoop(zBufferMaker->theWindow);

	delete zBufferMaker;

	return 0;
}
