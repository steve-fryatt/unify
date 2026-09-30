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
 * \file: log_font.c
 *
 * Log text handling implementation.
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

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/heap.h>

/* Application header files */

#include "log_font.h"

/**
 * The details of a log font instance
 */

struct log_font_block {
	/**
	 * The font size used in the window, in 16th of a point.
	 */
	int font_size;

	/**
	 * The line spacing used in the window, as a percentage of font size.
	 */
	int line_space;

	/**
	 * The minimum line height that we will provide (zero for none).
	 */
	int minimum_line_height;
};

/**
 * The font handle for normal text.
 */

static font_f log_font_normal_font = font_SYSTEM;

/**
 * The font handle for bold text.
 */

static font_f log_font_bold_font = font_SYSTEM;

/**
 * TODO
 */

static int log_font_line_height = 0;

/**
 * TODO
 */

static int log_font_baseline_offset = 0;

/**
 * The block to use when calling Font_ScanString.
 */

static font_scan_block log_font_scan_block = {
	.space.x = 0,
	.space.y = 0,
	.letter.x = 0,
	.letter.y = 0,
	.split_char = -1
};

/* Static function prototypes. */

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

struct log_font_block *log_font_create_instance(int mimimum_line_height, int font_size, int line_space)
{
	struct log_font_block *new = heap_alloc(sizeof(struct log_font_block));
	if (new == NULL)
		return NULL;

	new->minimum_line_height = mimimum_line_height;

	log_font_set_metrics(new, font_size, line_space);

	return new;
}

/**
 * Delete a log font instance.
 *
 * \param *instance		Pointer to the instance to be deleted.
 */

void log_font_delete_instance(struct log_font_block *instance)
{
	if (instance == NULL)
		return;

	heap_free(instance);
}

/**
 * Set the metrics of a log font instance.
 *
 * \param *instance		Pointer to the instance to be updated.
 * \param font_size		The new font size, in 16ths of a point.
 * \param line_space		The new line space to use, as a percentage
 *				of the font size.
 */

void log_font_set_metrics(struct log_font_block *instance, int font_size, int line_space)
{
	if (instance == NULL)
		return;

	instance->font_size = font_size;
	instance->line_space = line_space;
}

/**
 * Find the fonts required to plot into a window.
 *
 * \param *instance		Pointer to the log instance for which the fonts
 *				will be used.
 * \return			Pointer to an error block, or NULL if successful.
 */

os_error *log_font_find_fonts(struct log_font_block *instance)
{
	if (instance == NULL)
		return NULL;

	os_error *error = NULL;

	/* The normal font. */

	if (log_font_normal_font == font_SYSTEM && error == NULL) {
		error = xfont_find_font("Corpus.Medium", instance->font_size, instance->font_size, 0, 0,
				&log_font_normal_font, NULL, NULL);
		if (error != NULL)
			log_font_normal_font = font_SYSTEM;
	}

	/* The bold font. */

	if (log_font_bold_font == font_SYSTEM && error == NULL) {
		error = xfont_find_font("Corpus.Bold", instance->font_size, instance->font_size, 0, 0,
				&log_font_bold_font, NULL, NULL);
		if (error != NULL)
			log_font_bold_font = font_SYSTEM;
	}

	int line_height = 0;
	font_convertto_os(1000 * instance->font_size * instance->line_space / 1600, 0,
			&line_height, NULL);

	log_font_line_height = (line_height > instance->minimum_line_height) ?
			line_height : instance->minimum_line_height;

	log_font_baseline_offset = 0;
	font_convertto_os(1000 * instance->font_size * (instance->line_space - 100) / 1600, 0,
			&log_font_baseline_offset, NULL);

	if (log_font_line_height - line_height > 0)
		log_font_baseline_offset += log_font_line_height - line_height;

	return error;
}

/**
 * Lose the fonts used to plot into a window.
 */

void log_font_lose_fonts(void)
{
	if (log_font_normal_font != 0)
		font_lose_font(log_font_normal_font);

	if (log_font_bold_font != 0)
		font_lose_font(log_font_bold_font);

	log_font_normal_font = font_SYSTEM;
	log_font_bold_font = font_SYSTEM;
	log_font_line_height = 0;
	log_font_baseline_offset = 0;
}

/**
 * Return the required line height for the current font.
 *
 * log_font_find_fonts() must have been called before use.
 *
 * \return			The line spacing in OS units.
 */

int log_font_get_line_height(void)
{
	return log_font_line_height;
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

os_error *log_font_get_line_width(struct log_font_redraw *line_info, char *text, int *width)
{
	if (width != NULL)
		*width = 0;

	font_f font = (line_info->bold == TRUE) ? log_font_bold_font : log_font_normal_font;

	if (line_info == NULL || text == NULL || font == font_SYSTEM)
		return NULL;

	return log_font_get_text_width(font, text + line_info->offset, width);
}

/**
 * Calculate a base width for the log window, based on 80 columns of text.
 *
 * \param minimum_columns	The minimum number of columns required.
 * \return			The base width, in OS units.
 */

int log_font_get_base_width(int mininum_columns)
{
	char *text = "MMMMM";
	int normal_width = 0, bold_width = 0;

	if (log_font_get_text_width(log_font_normal_font, text, &normal_width))
		normal_width = -1;

	if (log_font_get_text_width(log_font_bold_font, text, &bold_width))
		bold_width = -1;

	if (normal_width == -1 && bold_width == -1)
		return mininum_columns * 16;

	return (normal_width > bold_width) ?
			normal_width * (mininum_columns / strlen(text)) :
			bold_width * (mininum_columns / strlen(text));
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

os_error *log_font_get_text_width(font_f font, char *text, int *width)
{
	if (width != NULL)
		*width = 0;

	if (text == NULL || font == font_SYSTEM)
		return NULL;

	os_error *error = xfont_scan_string(font, text, font_KERN | font_GIVEN_FONT | font_GIVEN_BLOCK | font_RETURN_BBOX,
			0x7fffffff, 0x7fffffff, &log_font_scan_block, NULL, 0, NULL, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	return xfont_convertto_os(log_font_scan_block.bbox.x1 - log_font_scan_block.bbox.x0, 0, width, NULL);
}

/**
 * Paint a line into a window.
 *
 * \param *line_info		Pointer to the line details.
 * \param *text			Pointer to the base of the text area.
 * \param *pos			Pointer to a coordinate block.
 * \return			Pointer to an error block, or NULL if successful.
 */

os_error *log_font_paint_text(struct log_font_redraw *line_info, char *text, os_coord *pos)
{
	if (line_info == NULL)
		return NULL;

	font_f font = (line_info->bold == TRUE) ? log_font_bold_font : log_font_normal_font;

	if (line_info == NULL || text == NULL || font == font_SYSTEM)
		return NULL;

	os_error *error = xcolourtrans_set_font_colours(font, os_COLOUR_VERY_LIGHT_GREY,
			line_info->colour, 14, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	return xfont_paint(font, text + line_info->offset, font_OS_UNITS | font_KERN | font_GIVEN_FONT,
			pos->x, pos->y + log_font_baseline_offset, NULL, NULL, 0);
}
