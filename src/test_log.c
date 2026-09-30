/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Unify:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: test_log.c
 *
 * Test log storage and display implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/colourtrans.h>
#include <oslib/font.h>
#include <oslib/os.h>
#include <oslib/osfile.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/event.h>
#include <sflib/heap.h>
#include <sflib/ihelp.h>
#include <sflib/saveas.h>
#include <sflib/templates.h>
#include <sflib/windows.h>

/* Application header files */

#include "test_log.h"

#include "log.h"
#include "log_font.h"
#include "flexutils.h"

/**
 * The log window menu entries
 */

#define LOG_MENU_SAVE_LOG 0

/* Structure definitions. */

/**
 * A test log instance.
 */

struct test_log_instance {
	/**
	 * The number of lines contained in the log window.
	 */
	unsigned line_count;

	/**
	 * The number of lines read by log_read_line().
	 */
	unsigned read_count;

	/**
	 * A flex block containing the log line redraw data.
	 */
	struct log_font_redraw *lines;

	/**
	 * The handle of the log window.
	 */
	wimp_w handle;

	/**
	 * The window title for the log.
	 */
	char *title;

	/**
	 * A flex block containing all of the log text.
	 */
	char *text;

	/**
	 * The length of the log text within the flex block.
	 */
	size_t length;

	/**
	 * The current allocated size of the flex block.
	 */
	size_t allocation;

	/**
	 * The log window display fonts.
	 */
	struct log_font_block *fonts;
};

/**
 * The minimum number of rows to show in a window.
 */

#define TEST_LOG_MINIMUM_ROWS 10

/**
 * The minimum number of columns to show in a window.
 */

#define TEST_LOG_MINIMUM_COLUMNS 80

/**
 * The inset of a log row.
 */

#define TEST_LOG_ROW_INSET 8

/**
 * The allocation unit for log space.
 */

#define TEST_LOG_ALLOCATION_UNIT 4098

/* Global variables. */

/**
 * Definition of the log window.
 */

static wimp_window *test_log_window_definition = NULL;

/**
 * The Save As dialogue box for logs.
 */

static struct saveas_block *test_log_saveas_dialogue = NULL;

/**
 * The window menu.
 */

static wimp_menu *test_log_window_menu = NULL;

/* Static function prototypes. */

static void test_log_close_handler(wimp_close *close);
static void test_log_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void test_log_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void test_log_redraw_handler(wimp_draw *redraw);
static void test_log_set_window_extent(struct test_log_instance *instance);
static osbool test_log_save_file(char *filename, osbool selection, void *data);

/**
 * Initialise the test log implementation.
 */

void test_log_initialise(void)
{
	test_log_window_definition = templates_load_window("TestLog");

	/* Set up the log window menu and its dialogues. */

	test_log_window_menu = templates_get_menu("TestLogWindowMenu");
	ihelp_add_menu(test_log_window_menu, "TestLogMenu");

	/* Set up the Save As dialogue. */

	test_log_saveas_dialogue = saveas_create_dialogue(FALSE, "file_fff", osfile_TYPE_TEXT, test_log_save_file);
}

/**
 * Create a new test log instance.
 *
 * \param *title		Pointer to the title to use for the log.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct test_log_instance *test_log_create_instance(char *title)
{
	/* Allocate the instance memory. */

	struct test_log_instance *instance = heap_alloc(sizeof(struct test_log_instance));
	if (instance == NULL)
		return NULL;

	instance->handle = NULL;
	instance->title = NULL;

	instance->lines = NULL;
	instance->line_count = 0;
	instance->read_count = 0;

	instance->allocation = TEST_LOG_ALLOCATION_UNIT;
	instance->length = 0;
	instance->text = NULL;

	instance->fonts = NULL;

	/* Set up the fonts. */

	instance->fonts = log_font_create_instance(0, 192, 130);
	if (instance->fonts == NULL) {
		test_log_delete_instance(instance);
		return NULL;
	}

	/* Store the window title. */

	instance->title = heap_strdup(title);
	if (instance->title == NULL) {
		test_log_delete_instance(instance);
		return NULL;
	}

	/* Allocate the flex blocks. */

	if (!flexutils_allocate((void **) &(instance->text), sizeof(char), instance->allocation)) {
		test_log_delete_instance(instance);
		return NULL;
	}

	debug_printf("Test Log 0x%x created...", instance);

	return instance;
}

/**
 * Destroy a test log instance.
 *
 * \param *instance		The instance to be deleted.
 */

void test_log_delete_instance(struct test_log_instance *instance)
{
	if (instance == NULL)
		return;

	/* Delete the windows. */

	if (instance->handle != NULL) {
		ihelp_remove_window(instance->handle);
		event_delete_window(instance->handle);
		wimp_delete_window(instance->handle);
	}

	/* Free the memory used. */

	if (instance->title != NULL)
		heap_free(instance->title);

	flexutils_free((void **) &(instance->lines));
	flexutils_free((void **) &(instance->text));

	debug_printf("Test Log 0x%x deleted...", instance);

	heap_free(instance);
}

/**
 * Open (or re-open) a window for a test log instance.
 *
 * \param *instance		The instance for which to open the window.
 */

void test_log_open_window(struct test_log_instance *instance)
{
	if (instance == NULL)
		return;

	if (instance->handle != NULL) {
		windows_open(instance->handle);
		return;
	}
	/* Create the new window. */

	test_log_window_definition->title_data.indirected_text.text =
			(instance->title != NULL) ? instance->title : "";
	test_log_window_definition->title_data.indirected_text.size =
			strlen(test_log_window_definition->title_data.indirected_text.text);

	os_error *error = xwimp_create_window(test_log_window_definition, &(instance->handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	ihelp_add_window(instance->handle, "TestLogWindow", NULL);

	event_add_window_user_data(instance->handle, instance);
	event_add_window_menu(instance->handle, test_log_window_menu);
	event_add_window_close_event(instance->handle, test_log_close_handler);
	event_add_window_menu_prepare(instance->handle, test_log_menu_prepare_handler);
	event_add_window_menu_warning(instance->handle, test_log_menu_warning_handler);
	event_add_window_redraw_event(instance->handle, test_log_redraw_handler);

	/* Open the window. */

	test_log_set_window_extent(instance);
	windows_open(instance->handle);
}

/**
 * Handle Close events on an instance window.
 *
 * \param *close		The Wimp Close data block.
 */

static void test_log_close_handler(wimp_close *close)
{
	struct test_log_instance *instance = event_get_window_user_data(close->w);

	if (instance == NULL || instance->handle == NULL)
		return;

	ihelp_remove_window(instance->handle);
	event_delete_window(instance->handle);
	wimp_delete_window(instance->handle);

	instance->handle = NULL;
}

/**
 * Handle requests to prepare the log menu.
 *
 * \param w			The window to which the menu belongs.
 * \param *menu			Pointer to the menu itself.
 * \param *pointer		Pointer to the pointer position data, or NULL
 *				on a reopening.
 */

static void test_log_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct window_instance *instance = event_get_window_user_data(w);
	if (instance == NULL || menu != test_log_window_menu)
		return;

	saveas_initialise_dialogue(test_log_saveas_dialogue, NULL, "DefLogFile", NULL, FALSE, FALSE, instance);
}

/**
 * Handle Message_MenuWarning events from the log menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param *warning		The submenu warning message data.
 */

static void test_log_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct window_instance *instance = event_get_window_user_data(w);
	if (instance == NULL || menu != test_log_window_menu)
		return;

	switch (warning->selection.items[0]) {
	case LOG_MENU_SAVE_LOG:
		saveas_prepare_dialogue(test_log_saveas_dialogue);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}

/**
 * Handle Redraw events on an log instance window.
 *
 * \param *redraw		The Wimp Redraw data block.
 */

static void test_log_redraw_handler(wimp_draw *redraw)
{
	struct test_log_instance *instance = event_get_window_user_data(redraw->w);

	log_font_find_fonts(instance->fonts);
	int row_height = log_font_get_line_height();

	/* Perform the redraw. */

	osbool more = wimp_redraw_window(redraw);

	/* Work out the redraw origin. */

	int ox = (instance != NULL) ? redraw->box.x0 - redraw->xscroll : 0;
	int oy = (instance != NULL) ? redraw->box.y1 - redraw->yscroll : 0;

	os_coord pos = { .x = ox + TEST_LOG_ROW_INSET };

	while (more) {
		if (instance != NULL && instance->lines != NULL) {
			int top = (oy - redraw->clip.y1) / row_height;
			if (top < 0)
				top = 0;

			int base = (oy - redraw->clip.y0) / row_height;
			if (base >= instance->line_count)
				base = instance->line_count - 1;

			for (int y = top; y <= base; y++) {
				pos.y = oy - ((y + 1) * row_height);
				log_font_paint_text(instance->lines + y, instance->text, &pos);
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	log_font_lose_fonts();
}

/**
 * Add a block of text to the log instance. Text may contain control characters,
 * and does not need to be terminated: the specified number of bytes will be
 * copied.
 *
 * \param *instance		Pointer to the instance to take the text.
 * \param *content		Pointer to the content to be added.
 * \param length		The number of bytes in the content.
 */

void test_log_add_text(struct test_log_instance *instance, char *content, size_t length)
{
	if (instance == NULL || content == NULL || length == 0 || instance->length < 0)
		return;

	/* Make sure that we have enough space. Add 1 to the space so that at the
	 * end we have a byte left to terminate the buffer if we have to.
	 */

	if (instance->length + length + 1 >= instance->allocation) {
		size_t new_space = instance->allocation;

		while (new_space <= instance->length + length + 1)
			new_space += TEST_LOG_ALLOCATION_UNIT;

		if (flexutils_resize((void **) &(instance->text), sizeof(char), new_space))
			instance->allocation = new_space;
	}

	if (instance->length + length + 1 >= instance->allocation)
		return;

	/* Copy the text into the buffer. */

	debug_printf("Adding log text...");

	for (int i = 0; i < length; i++)
		instance->text[i + instance->length] = content[i];

	instance->length += length;
}

/**
 * Set the extent of a test log window.
 *
 * \param *instance		Pointer to the instance to be updated.
 */

static void test_log_set_window_extent(struct test_log_instance *instance)
{
	if (instance == NULL)
		return;

	log_font_find_fonts(instance->fonts);

	/* Get the window width. */

	int window_width = log_font_get_base_width(TEST_LOG_MINIMUM_COLUMNS);

	for (int i = 0; i < instance->line_count; i++) {
		int line_width = 0;
		if (log_font_get_line_width(instance->lines + i, instance->text, &line_width))
			continue;

		if (line_width > window_width)
			window_width = line_width;
	}

	window_width += 3 * TEST_LOG_ROW_INSET;

	/* Get the window height. */

	int window_height = 3 * TEST_LOG_ROW_INSET + log_font_get_line_height() *
			((instance->line_count > 10) ? instance->line_count : TEST_LOG_MINIMUM_ROWS);

	log_font_lose_fonts();

	os_box extent = {
		.x0 = 0,
		.x1 = window_width,
		.y0 = -window_height,
		.y1 = 0
	};

	wimp_set_extent(instance->handle, &extent);
}

/**
 * Save the test log to a file on disc.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		TRUE if "selection" was ticked in the dialogue.
 * \param *data			The saveas client data, which is a pointer to
 *				the log instance.
 * \return			TRUE if the save was successful; else FALSE.
 */

static osbool test_log_save_file(char *filename, osbool selection, void *data)
{
	struct test_log_instance *instance = data;
	if (instance == NULL || filename == NULL)
		return FALSE;

	FILE *file = fopen(filename, "w");
	if (file == NULL)
		return FALSE;

	osbool written = test_log_write_to_file(instance, file);

	fclose(file);

	return written;
}

/**
 * Write a test log to a file handle.
 *
 * \param *instance		Pointer to the log instance to be written.
 * \param *file			Pointer to the file handle to write to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool test_log_write_to_file(struct test_log_instance *instance, FILE *file)
{
	if (instance == NULL || instance->lines == NULL || file == NULL)
		return FALSE;

	for (unsigned line = 0; line < instance->line_count; line++) {
		if (fputs(instance->text + instance->lines[line].offset, file) == EOF || fputc('\n', file) == EOF)
			return FALSE;
	}

	return TRUE;
}
