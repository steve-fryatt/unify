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
	unsigned offset;	/**< Offset into the text area for the text.	*/
	os_colour colour;	/**< The colour of the line.			*/
	osbool bold;		/**< Should the text be bold?			*/
};

/**
 * A log instance.
 */

struct log_instance {
	/**
	 * The number of lines contained in the log window.
	 */
	int line_count;

	/**
	 * A flex block containing the log line redraw data.
	 */
	struct log_redraw *lines;

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
	 * The font size used in the window, in 16th of a point.
	 */
	int font_size;

	/**
	 * The line spacing used in the window, as a percentage of font size.
	 */
	int linespace;
};

/**
 * The minimum number of rows to show in a window.
 */

#define LOG_MINIMUM_ROWS 10

/**
 * The minimum number of columns to show in a window.
 */

#define LOG_MINIMUM_COLUMNS 80

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

/**
 * The block to use when calling Font_ScanString.
 */

static font_scan_block log_scan_block = {
	.space.x = 0,
	.space.y = 0,
	.letter.x = 0,
	.letter.y = 0,
	.split_char = -1
};

/* Static function prototypes. */

static void log_close_handler(wimp_close *close);
static void log_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void log_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void log_redraw_handler(wimp_draw *redraw);
static void log_set_window_extent(struct log_instance *instance);
static osbool log_save_file(char *filename, osbool selection, void *data);
static os_error *log_find_fonts(struct log_instance *instance);
static void log_lose_fonts(void);
static int log_get_linespace(struct log_instance *instance);
static os_error *log_get_line_width(struct log_redraw *line_info, char *text, int *width);
static int log_get_base_width(void);
static os_error *log_get_text_width(font_f font, char *text, int *width);
static os_error *log_paint_text(struct log_redraw *line_info, char *text, os_coord *pos);

/**
 * Initialise the log implementation.
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
 * \param *title		Pointer to the title to use for the log.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct log_instance *log_create_instance(char *title)
{
	/* Allocate the instance memory. */

	struct log_instance *instance = heap_alloc(sizeof(struct log_instance));
	if (instance == NULL)
		return NULL;

	instance->handle = NULL;
	instance->title = NULL;

	instance->lines = NULL;
	instance->line_count = 0;

	instance->allocation = LOG_ALLOCATION_UNIT;
	instance->length = 0;
	instance->text = NULL;

	instance->font_size = 192; // 12pt
	instance->linespace = 130;

	/* Store the window title. */

	instance->title = heap_strdup(title);
	if (instance->title == NULL) {
		log_delete_instance(instance);
		return NULL;
	}

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

	if (instance->title != NULL)
		heap_free(instance->title);

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

	log_window_definition->title_data.indirected_text.text =
			(instance->title != NULL) ? instance->title : "";
	log_window_definition->title_data.indirected_text.size =
			strlen(log_window_definition->title_data.indirected_text.text);

	os_error *error = xwimp_create_window(log_window_definition, &(instance->handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	ihelp_add_window(instance->handle, "LogWindow", NULL);

	event_add_window_user_data(instance->handle, instance);
	event_add_window_menu(instance->handle, log_window_menu);
	event_add_window_close_event(instance->handle, log_close_handler);
	event_add_window_menu_prepare(instance->handle, log_menu_prepare_handler);
	event_add_window_menu_warning(instance->handle, log_menu_warning_handler);
	event_add_window_redraw_event(instance->handle, log_redraw_handler);

	/* Open the window. */

	log_set_window_extent(instance);
	windows_open(instance->handle);
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

	log_find_fonts(instance);
	int row_height = log_get_linespace(instance);

	/* Perform the redraw. */

	osbool more = wimp_redraw_window(redraw);

	/* Work out the redraw origin. */

	int ox = (instance != NULL) ? redraw->box.x0 - redraw->xscroll : 0;
	int oy = (instance != NULL) ? redraw->box.y1 - redraw->yscroll : 0;

	os_coord pos = { .x = ox + LOG_ROW_INSET };

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
				log_paint_text(instance->lines + y, instance->text, &pos);
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	log_lose_fonts();
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

void log_add_text(struct log_instance *instance, char *content, size_t length)
{
	if (instance == NULL || content == NULL || length == 0 || instance->length < 0)
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
 * Complete the addition of text to the log instance. This will cause the
 * content to be formatted and prepared for display.
 *
 * \param *instance		Pointer to the instance to be completed.
 */

void log_finish_text(struct log_instance *instance)
{
	if (instance == NULL || instance->text == NULL || instance->length < 0)
		return;

	/* Terminate the log content with a zero byte, in case there isn't one. */

	if ((instance->length + 1) >= instance->allocation)
		return;

	instance->text[instance->length++] = '\0';

	/* Shrink the flex allocation down to the size required. */

	if (flexutils_resize((void **) &(instance->text), sizeof(char), instance->length))
		instance->allocation = instance->length;

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

	/* Allocate space for the redraw data and populate it. */

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

	/* Close the log off to future updates. */

	instance->length = -1;
}

/**
 * Set the extent of a log window.
 *
 * \param *instance		Pointer to the instance to be updated.
 */

static void log_set_window_extent(struct log_instance *instance)
{
	if (instance == NULL)
		return;

	log_find_fonts(instance);

	/* Get the window width. */

	int window_width = log_get_base_width();

	for (int i = 0; i < instance->line_count; i++) {
		int line_width = 0;
		if (log_get_line_width(instance->lines + i, instance->text, &line_width))
			continue;

		if (line_width > window_width)
			window_width = line_width;
	}

	window_width += 3 * LOG_ROW_INSET;

	/* Get the window height. */

	int window_height = 3 * LOG_ROW_INSET + log_get_linespace(instance) *
			((instance->line_count > 10) ? instance->line_count : LOG_MINIMUM_ROWS);

	log_lose_fonts();

	os_box extent = {
		.x0 = 0,
		.x1 = window_width,
		.y0 = -window_height,
		.y1 = 0
	};

	wimp_set_extent(instance->handle, &extent);
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
 * \param *instance		Pointer to the log instance for which the fonts
 *				will be used.
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *log_find_fonts(struct log_instance *instance)
{
	if (instance == NULL)
		return NULL;

	os_error *error = NULL;

	if (log_normal_font == font_SYSTEM && error == NULL) {
		error = xfont_find_font("Corpus.Medium", instance->font_size, instance->font_size, 0, 0,
				&log_normal_font, NULL, NULL);
		if (error != NULL)
			log_normal_font = font_SYSTEM;
	}

	if (log_bold_font == font_SYSTEM && error == NULL) {
		error = xfont_find_font("Corpus.Bold", instance->font_size, instance->font_size, 0, 0,
				&log_bold_font, NULL, NULL);
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
 * Return the required line spacing for the current font.
 *
 * \param *instance		Pointer to the log instance for which the fonts
 *				will be used.
 * \return			The line spacing in OS units.
 */

static int log_get_linespace(struct log_instance *instance)
{
	if (instance == NULL)
		return 32; // A value that might work, at a push.

	int linespace = 0;

	font_convertto_os(1000 * (instance->font_size / 16) * instance->linespace / 100, 0, &linespace, NULL);

	return linespace;
}

/**
 * Calculate the width of a line of text in the current font.
 *
 * \param *line_info		Pointer to the line details.
 * \param *text			Pointer to the base of the text area.
 * \param *width		Pointer to a variable to take the width of the
 *				line in OS Units.
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *log_get_line_width(struct log_redraw *line_info, char *text, int *width)
{
	if (width != NULL)
		*width = 0;

	font_f font = (line_info->bold == TRUE) ? log_bold_font : log_normal_font;

	if (line_info == NULL || text == NULL || font == font_SYSTEM)
		return NULL;

	return log_get_text_width(font, text + line_info->offset, width);
}

/**
 * Calculate a base width for the log window, based on 80 columns of text.
 *
 * \return			The base width, in OS units.
 */

static int log_get_base_width(void)
{
	char *text = "MMMMM";
	int normal_width = 0, bold_width = 0;

	if (log_get_text_width(log_normal_font, text, &normal_width))
		normal_width = -1;

	if (log_get_text_width(log_bold_font, text, &bold_width))
		bold_width = -1;

	if (normal_width == -1 && bold_width == -1)
		return LOG_MINIMUM_COLUMNS * 16;

	return (normal_width > bold_width) ?
			normal_width * (LOG_MINIMUM_COLUMNS / strlen(text)) :
			bold_width * (LOG_MINIMUM_COLUMNS / strlen(text));
}

/**
 * Calculate the width of a piece of text in a given font.
 *
 * \param font			The handle of the font to use.
 * \param *text			Pointer to the text to check.
 * \param *width		Pointer to a variable to take the width of the
 *				line in OS Units.
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *log_get_text_width(font_f font, char *text, int *width)
{
	if (width != NULL)
		*width = 0;

	if (text == NULL || font == font_SYSTEM)
		return NULL;

	os_error *error = xfont_scan_string(font, text, font_KERN | font_GIVEN_FONT | font_GIVEN_BLOCK | font_RETURN_BBOX,
			0x7fffffff, 0x7fffffff, &log_scan_block, NULL, 0, NULL, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	return xfont_convertto_os(log_scan_block.bbox.x1 - log_scan_block.bbox.x0, 0, width, NULL);
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
	if (line_info == NULL)
		return NULL;

	font_f font = (line_info->bold == TRUE) ? log_bold_font : log_normal_font;

	if (line_info == NULL || text == NULL || font == font_SYSTEM)
		return NULL;

	os_error *error = xcolourtrans_set_font_colours(font, os_COLOUR_VERY_LIGHT_GREY,
			line_info->colour, 14, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	return xfont_paint(font, text + line_info->offset, font_OS_UNITS | font_KERN | font_GIVEN_FONT,
			pos->x, pos->y, NULL, NULL, 0);
}
