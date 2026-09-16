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
 * \file: log.c
 *
 * Log storage and display implementation.
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

#include "log.h"

#include "flexutils.h"

/**
 * The log window menu entries
 */

#define LOG_MENU_SAVE_LOG 0

/* Structure definitions. */

/**
 * A line redraw record.
 */

struct log_redraw {
	unsigned offset;
	os_colour colour;
	osbool bold;
};

/**
 * A log instance.
 */

struct log_instance {
	int line_count;

	struct log_redraw *lines;

	wimp_w handle;

	char *text;

	size_t length;

	size_t allocation;
};

/***
 * The height of a log row.
 */

#define LOG_ROW_HEIGHT 32

/**
 * The inset of a log row.
 */

#define LOG_ROW_INSET 8

/**
 * The allocation unit for log space.
 */

#define LOG_ALLOCATION_UNIT 4098

/* Global variables. */

/**
 * Definition of the log window.
 */

static wimp_window *log_window_definition = NULL;

/**
 * The Save As dialogue box for logs.
 */

static struct saveas_block *log_saveas_dialogue = NULL;

/**
 * The window menu.
 */

static wimp_menu *log_window_menu = NULL;

/**
 * The font handle for normal text.
 */

static font_f log_normal_font = font_SYSTEM;

/**
 * The font handle for bold text.
 */

static font_f log_bold_font = font_SYSTEM;

/* Static function prototypes. */

static void log_close_handler(wimp_close *close);
static void log_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void log_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void log_redraw_handler(wimp_draw *redraw);
static osbool log_save_file(char *filename, osbool selection, void *data);
static os_error *log_find_fonts(void);
static void log_lose_fonts(void);
static os_error *log_paint_text(struct log_redraw *line_info, char *text, os_coord *pos);

/**
 * Initialise the log implementation.
 *
 * \param task_handle		The handle of the task.
 */

void log_initialise(void)
{
	log_window_definition = templates_load_window("Log");

	/* Set up the log window menu and its dialogues. */

	log_window_menu = templates_get_menu("LogWindowMenu");
	ihelp_add_menu(log_window_menu, "LogMenu");

	/* Set up the Save As dialogue. */

	log_saveas_dialogue = saveas_create_dialogue(FALSE, "file_fff", osfile_TYPE_TEXT, log_save_file);
}

/**
 * Create a new log instance.
 *
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct log_instance *log_create_instance(void)
{
	/* Allocate the instance memory. */

	struct log_instance *instance = heap_alloc(sizeof(struct log_instance));
	if (instance == NULL)
		return NULL;

	instance->handle = NULL;

	instance->lines = NULL;
	instance->line_count = 0;

	instance->allocation = LOG_ALLOCATION_UNIT;
	instance->length = 0;
	instance->text = NULL;

	/* Allocate the flex blocks. */

	if (!flexutils_allocate((void **) &(instance->text), sizeof(char), instance->allocation)) {
		log_delete_instance(instance);
		return NULL;
	}

	debug_printf("Log 0x%x created...", instance);

	return instance;
}

/**
 * Destroy a log instance.
 *
 * \param *instance		The instance to be deleted.
 */

void log_delete_instance(struct log_instance *instance)
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

	flexutils_free((void **) &(instance->lines));
	flexutils_free((void **) &(instance->text));

	debug_printf("Log 0x%x deleted...", instance);

	heap_free(instance);
}

/**
 * Open (or re-open) a window for a log instance.
 *
 * \param *instance		The instance for which to open the window.
 */

void log_open_window(struct log_instance *instance)
{
	if (instance == NULL)
		return;

	if (instance->handle != NULL) {
		windows_open(instance->handle);
		return;
	}
	/* Create the new window. */

	os_error *error = xwimp_create_window(log_window_definition, &(instance->handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	ihelp_add_window(instance->handle, "LogWindow", NULL);

	event_add_window_user_data(instance->handle, instance);
	event_add_window_menu(instance->handle, log_window_menu);
//	event_add_window_open_event(instance->handle, window_open_handler);
	event_add_window_close_event(instance->handle, log_close_handler);
//	event_add_window_mouse_event(instance->handle, window_click_handler);
	event_add_window_menu_prepare(instance->handle, log_menu_prepare_handler);
	event_add_window_menu_warning(instance->handle, log_menu_warning_handler);
//	event_add_window_menu_selection(instance->handle, log_menu_selection_handler);
//	event_add_window_menu_close(instance->handle, window_menu_close);
	event_add_window_redraw_event(instance->handle, log_redraw_handler);
//	event_add_window_scroll_event(instance->handle, window_scroll_handler);

	/* Open the windows. */

//	wimp_window_state window = { .w = instance->handle };
//	wimp_get_window_state(&window);
//	window_recalculate_columns(instance, (wimp_open *) &window);
//	window.next = wimp_TOP;
//	wimp_open_window((wimp_open *) &window);

	windows_open(instance->handle);

//	windows_open_nested_as_toolbar(instance->pane_handle, instance->handle, instance->pane_size, FALSE);
//	window_position_toolbar_icons((wimp_open *) &window, instance);
}

/**
 * Handle Close events on an instance window.
 *
 * \param *close		The Wimp Close data block.
 */

static void log_close_handler(wimp_close *close)
{
	struct log_instance *instance = event_get_window_user_data(close->w);

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

static void log_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct window_instance *instance = event_get_window_user_data(w);
	if (instance == NULL || menu != log_window_menu)
		return;

	saveas_initialise_dialogue(log_saveas_dialogue, NULL, "DefLogFile", NULL, FALSE, FALSE, instance);
}

/**
 * Handle Message_MenuWarning events from the log menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param *warning		The submenu warning message data.
 */

static void log_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct window_instance *instance = event_get_window_user_data(w);
	if (instance == NULL || menu != log_window_menu)
		return;

	switch (warning->selection.items[0]) {
	case LOG_MENU_SAVE_LOG:
		saveas_prepare_dialogue(log_saveas_dialogue);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}

/**
 * Handle Redraw events on an log instance window.
 *
 * \param *redraw		The Wimp Redraw data block.
 */

static void log_redraw_handler(wimp_draw *redraw)
{
	struct log_instance *instance = event_get_window_user_data(redraw->w);

	log_find_fonts();

	/* Perform the redraw. */

	osbool more = wimp_redraw_window(redraw);

	/* Work out the redraw origin. */

	int ox = (instance != NULL) ? redraw->box.x0 - redraw->xscroll : 0;
	int oy = (instance != NULL) ? redraw->box.y1 - redraw->yscroll : 0;

	os_coord pos = { .x = ox + LOG_ROW_INSET };

	while (more) {
		if (instance != NULL && instance->lines != NULL) {
			int top = (oy - redraw->clip.y1) / LOG_ROW_HEIGHT;
			if (top < 0)
				top = 0;

			int base = (oy - redraw->clip.y0) / LOG_ROW_HEIGHT;
			if (base >= instance->line_count)
				base = instance->line_count - 1;

			debug_printf("Redrawing lines %d to %d", top, base);

			for (int y = top; y <= base; y++) {
				pos.y = oy - ((y + 1) * LOG_ROW_HEIGHT);
				log_paint_text(instance->lines + y, instance->text, &pos);
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	log_lose_fonts();
}

/**
 * TODO
 */

void log_add_text(struct log_instance *instance, char *content, size_t length)
{
	if (instance == NULL || content == NULL || length == 0)
		return;

	/* Make sure that we have enough space. Add 1 to the space so that at the
	 * end we have a byte left to terminate the buffer if we have to.
	 */

	if (instance->length + length + 1 >= instance->allocation) {
		size_t new_space = instance->allocation;

		while (new_space <= instance->length + length + 1)
			new_space += LOG_ALLOCATION_UNIT;

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
 * TODO
 */

void log_finish_text(struct log_instance *instance)
{
	if (instance == NULL || instance->text == NULL)
		return;

	if ((instance->length + 1) >= instance->allocation)
		return;

	instance->text[instance->length++] = '\0';

	debug_printf("The log: %s", instance->text);

	/* Find line endings. */

	int lines = (instance->length > 0) ? 1 : 0;

	for (unsigned i = 0; i < (instance->length - 1); i++) {
		if (instance->text[i] == '\r' && instance->text[i+1] == '\n') {
			instance->text[i++] = '\0';
			instance->text[i] = '\0';
			lines++;
		} else if (instance->text[i] == '\n' && instance->text[i+1] == '\r') {
			instance->text[i++] = '\0';
			instance->text[i] = '\0';
			lines++;
		} else if (instance->text[i] == '\r' || instance->text[i] == '\n') {
			instance->text[i] = '\0';
			lines++;
		}
	}

	debug_printf("Found %d log lines", lines);

	if (!flexutils_allocate((void **) &(instance->lines), sizeof(struct log_redraw), lines)) {
		instance->lines = NULL;
		return;
	}

	instance->line_count = lines;

	unsigned i = 0;
	int line = 0;

	while (i < instance->length && line < lines) {
		instance->lines[line].offset = i;
		instance->lines[line].bold = FALSE;
		instance->lines[line].colour = os_COLOUR_BLACK;
		line++;

		while (i < instance->length && instance->text[i] != '\0')
			i++;

		while (i < instance->length && instance->text[i] == '\0')
			i++;
	}
}

/**
 * Save the log to a file on disc.
 *
 * \param *filename		Pointer to the filename to save to.
 * \param selection		TRUE if "selection" was ticked in the dialogue.
 * \param *data			The saveas client data, which is a pointer to
 *				the log instance.
 * \return			TRUE if the save was successful; else FALSE.
 */

static osbool log_save_file(char *filename, osbool selection, void *data)
{
	struct log_instance *instance = data;
	if (instance == NULL || filename == NULL)
		return FALSE;

	FILE *file = fopen(filename, "w");
	if (file == NULL)
		return FALSE;

	osbool written = log_write_to_file(instance, file);

	fclose(file);

	return written;
}

/**
 * Write a log to a file handle.
 *
 * \param *instance		Pointer to the log instance to be written.
 * \param *file			Pointer to the file handle to write to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool log_write_to_file(struct log_instance *instance, FILE *file)
{
	if (instance == NULL || instance->lines == NULL || file == NULL)
		return FALSE;

	for (int line = 0; line < instance->line_count; line++) {
		if (fputs(instance->text + instance->lines[line].offset, file) == EOF || fputc('\n', file) == EOF)
			return FALSE;
	}

	return TRUE;
}

/**
 * Find the fonts required to plot into a window.
 *
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *log_find_fonts(void)
{
	os_error *error = NULL;
	int size = 192; // 12 pt

	if (log_normal_font == 0 && error == NULL) {
		error = xfont_find_font("Corpus.Medium", size, size, 0, 0, &log_normal_font, NULL, NULL);
		if (error != NULL)
			log_normal_font = font_SYSTEM;
	}

	if (log_bold_font == 0 && error == NULL) {
		error = xfont_find_font("Corpus.Bold", size, size, 0, 0, &log_bold_font, NULL, NULL);
		if (error != NULL)
			log_bold_font = font_SYSTEM;
	}

	return error;
}

/**
 * Lose the fonts used to plot into a window.
 */

static void log_lose_fonts(void)
{
	if (log_normal_font != 0)
		font_lose_font(log_normal_font);

	if (log_bold_font != 0)
		font_lose_font(log_bold_font);

	log_normal_font = font_SYSTEM;
	log_bold_font = font_SYSTEM;
}

/**
 * Paint a line into a window.
 *
 * \param *line_info		Pointer to the line details.
 * \param *text			Pointer to the base of the text area.
 * \param *pos			Pointer to a coordinate block.
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *log_paint_text(struct log_redraw *line_info, char *text, os_coord *pos)
{
	os_error *error;
	font_f font;

	if (line_info == NULL)
		return NULL;

	font = (line_info->bold == TRUE) ? log_bold_font : log_normal_font;

	if (text == NULL || font == font_SYSTEM)
		return NULL;

	error = xcolourtrans_set_font_colours(font, os_COLOUR_VERY_LIGHT_GREY, line_info->colour, 14, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	return xfont_paint(font, text + line_info->offset, font_OS_UNITS | font_KERN | font_GIVEN_FONT, pos->x, pos->y, NULL, NULL, 0);
}
