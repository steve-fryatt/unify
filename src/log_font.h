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
 * \file: log.h
 *
 * Log text handling interface.
 */

#ifndef UNIFY_LOG_FONT
#define UNIFY_LOG_FONT

#include <stdio.h>
#include <oslib/os.h>
#include <oslib/types.h>

/* Structure definitions. */

/**
 * A line redraw record.
 */

struct log_font_redraw {
	unsigned offset;	/**< Offset into the text area for the text.	*/
	os_colour colour;	/**< The colour of the line.			*/
	osbool bold;		/**< Should the text be bold?			*/
};

/**
 * A log font instance.
 */

struct log_font_block;

/**
 * Create a new log font instance.
 *
 * \param mimimum_line_height	The minimum height of a line, in OS units.
 * \param font_size		The initial font size to use, in 16ths of a
 *				point.
 * \param line_space		The initial line space to use, as a percentage
 *				of the font size.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct log_font_block *log_font_create_instance(int mimimum_line_height, int font_size, int line_space);

/**
 * Delete a log font instance.
 *
 * \param *instance		Pointer to the instance to be deleted.
 */

void log_font_delete_instance(struct log_font_block *instance);

/**
 * Set the metrics of a log font instance.
 *
 * \param *instance		Pointer to the instance to be updated.
 * \param font_size		The new font size, in 16ths of a point.
 * \param line_space		The new line space to use, as a percentage
 *				of the font size.
 */

void log_font_set_metrics(struct log_font_block *instance, int font_size, int line_space);

/**
 * Find the fonts required to plot into a window.
 *
 * \param *instance		Pointer to the log instance for which the fonts
 *				will be used.
 * \return			Pointer to an error block, or NULL if successful.
 */

os_error *log_font_find_fonts(struct log_font_block *instance);

/**
 * Lose the fonts used to plot into a window.
 */

void log_font_lose_fonts(void);

/**
 * Return the required line height for the current font.
 *
 * log_font_find_fonts() must have been called before use.
 *
 * \return			The line spacing in OS units.
 */

int log_font_get_line_height(void);

/**
 * Calculate the width of a line of text in the current font.
 *
 * \param *line_info		Pointer to the line details.
 * \param *text			Pointer to the base of the text area.
 * \param *width		Pointer to a variable to take the width of the
 *				line in OS Units.
 * \return			Pointer to an error block, or NULL if successful.
 */

os_error *log_font_get_line_width(struct log_font_redraw *line_info, char *text, int *width);

/**
 * Calculate a base width for the log window, based on 80 columns of text.
 *
 * \param minimum_columns	The minimum number of columns required.
 * \return			The base width, in OS units.
 */

int log_font_get_base_width(int mininum_columns);

/**
 * Calculate the width of a piece of text in a given font.
 *
 * \param font			The handle of the font to use.
 * \param *text			Pointer to the text to check.
 * \param *width		Pointer to a variable to take the width of the
 *				line in OS Units.
 * \return			Pointer to an error block, or NULL if successful.
 */

os_error *log_font_get_text_width(font_f font, char *text, int *width);

/**
 * Paint a line into a window.
 *
 * \param *line_info		Pointer to the line details.
 * \param *text			Pointer to the base of the text area.
 * \param *pos			Pointer to a coordinate block.
 * \return			Pointer to an error block, or NULL if successful.
 */

os_error *log_font_paint_text(struct log_font_redraw *line_info, char *text, os_coord *pos);

#endif
