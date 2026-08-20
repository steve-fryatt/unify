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
 * \file: window.c
 *
 * Text Window implementation.
 */

/* ANSI C header files */

#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/colourtrans.h"
#include "oslib/font.h"
#include "oslib/os.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "window.h"

/* Constant definitions. */

/**
 * The size of a line text buffer.
 */

#define WINDOW_LINE_BUFFER_LEN 256

/**
 * The minimum number of lines to show in a window.
 */

#define WINDOW_MINIMUM_SIZE 10

/**
 * The icon templates.
 */

#define WINDOW_TEMPLATE_ICON_NAME 0
#define WINDOW_TEMPLATE_ICON_DETAIL 1

/* Structure definitions. */

/**
 * A Text Window Instance Definition.
 */

struct window_instance {
	struct window_definition *definition;	/**< The window defintion.		*/
	void *client_data;			/**< The client data pointer.		*/

	wimp_w handle;		/**< The Wimp handle of the window.			*/
	wimp_w pane_handle;	/**< The Wimp handle of the pane.			*/
	int pane_size;		/**< The height of a toolbar pane, in OS units.		*/
	int width;		/**< The window width in OS units.			*/

	int entries;				/**< The number of items in the window.	*/
};

/* Global variables. */

/**
 * Definition of the list window.
 */

static wimp_window *window_definition = NULL;

/**
 * Definition of the list window pane.
 */

static wimp_window *window_pane_definition = NULL;

/**
 * The font handle for normal text.
 */

static font_f window_normal_font = font_SYSTEM;

/**
 * The font handle for bold text.
 */

static font_f window_bold_font = font_SYSTEM;

/* Static function prototypes. */

static void window_close_handler(wimp_close *close);
static void window_redraw_handler(wimp_draw *redraw);
static void window_scroll_handler(wimp_scroll *scroll);

//static void window_format_numeric_data(struct window_redraw *value, char *buffer, size_t length);
//static os_error *window_find_fonts(void);
//static void window_lose_fonts(void);
//static os_error *window_paint_text(struct window_redraw *line_info, char *text, os_coord *pos);

/**
 * Initialise the text window.
 *
 * \param *sprites		Pointet to the user sprite area.
 */

void window_initialise(osspriteop_area *sprites)
{
	window_definition = templates_load_window("List");
	window_definition->sprite_area = sprites;
	window_definition->icon_count = 0;
	window_pane_definition = templates_load_window("ListPane");
}


/**
 * Create a new window instance.
 *
 * \param *pane_definition	Pointer to the window definition.
 * \param *client_data		Pointer to the client data, or NULL for none.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct window_instance *window_create_instance(struct window_definition *definition, void *client_data)
{
	if (definition == NULL)
		return NULL;

	/* Allocate the instance memory. */

	struct window_instance *instance = heap_alloc(sizeof(struct window_instance));
	if (instance == NULL)
		return NULL;

	instance->definition = definition;
	instance->client_data = client_data;
	instance->handle = NULL;
	instance->pane_handle = NULL;
	instance->width = 1200;
	instance->pane_size = 0;
	instance->entries = 4;

	/* Create the new window. */

	os_error *error = xwimp_create_window(window_definition, &(instance->handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		window_delete_instance(instance);
		return NULL;
	}

	/* Create the new window pane. */

//	instance->width = pane_definition->visible.x1 - pane_definition->visible.x0;
	instance->pane_size = window_pane_definition->visible.y1 - window_pane_definition->visible.y0;
	windows_place_as_toolbar(window_definition, window_pane_definition, instance->pane_size);

	error = xwimp_create_window(window_pane_definition, &(instance->pane_handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		window_delete_instance(instance);
		return NULL;
	}

	event_add_window_user_data(instance->handle, instance);
	event_add_window_close_event(instance->handle, window_close_handler);
	event_add_window_redraw_event(instance->handle, window_redraw_handler);
	event_add_window_scroll_event(instance->handle, window_scroll_handler);

	windows_open(instance->handle);
	windows_open_nested_as_toolbar(instance->pane_handle, instance->handle, instance->pane_size, FALSE);

	return instance;
}


/**
 * Destroy a text window instance.
 *
 * \param *instance		The instance to be deleted.
 */

void window_delete_instance(struct window_instance *instance)
{
	if (instance == NULL)
		return;

	/* Delete the windows. */

	if (instance->handle != NULL)
		wimp_delete_window(instance->handle);

	if (instance->pane_handle != NULL)
		wimp_delete_window(instance->pane_handle);

	/* Free the memory used. */

	heap_free(instance);
}

/**
 * Handle Close events on an instance window.
 *
 * \param *close		The Wimp Close data block.
 */

static void window_close_handler(wimp_close *close)
{
	struct window_instance *instance = event_get_window_user_data(close->w);

	if (instance != NULL && instance->definition->callback_close != NULL)
		instance->definition->callback_close(instance->client_data);
}



static void window_redraw_handler(wimp_draw *redraw)
{
	struct window_instance *instance = event_get_window_user_data(redraw->w);

	/* Perform the redraw. */

	osbool more = wimp_redraw_window(redraw);

	/* Work out the redraw origin. */

	int oy = (instance != NULL) ? redraw->box.y1 - redraw->yscroll : 0;

	while (more) {
		if (instance != NULL) {
			int top = ((oy - redraw->clip.y1) - instance->pane_size) / WINDOW_ROW_HEIGHT;
	//		int top = WINDOW_REDRAW_TOP(instance->pane_size, oy - redraw->clip.y1);
			if (top < 0)
				top = 0;

			int base = ((oy - redraw->clip.y0) - instance->pane_size) / WINDOW_ROW_HEIGHT;
	//		int base = WINDOW_REDRAW_BASE(instance->pane_size, oy - redraw->clip.y0);
	//		if (base > instance->entries)
	//			base = instance->entries;

			wimp_icon *name_icon = window_definition->icons + WINDOW_TEMPLATE_ICON_NAME;

			for (int y = top; y <= base; y++) {
				if (instance->definition->callback_redraw == NULL)
					break;

				struct window_line content;

				if (instance->definition->callback_redraw(y, &content, instance->client_data) == FALSE)
					break;

				name_icon->extent.y1 = -((y * WINDOW_ROW_HEIGHT) + WINDOW_ROW_GUTTER + instance->pane_size);
				name_icon->extent.y0 = window_definition->icons[0].extent.y1 - WINDOW_ROW_ICON_HEIGHT;

	//			window_definition->icons[0].extent.y0 = WINDOW_ROW_Y0(instance->pane_size, y);
	//			window_definition->icons[0].extent.y1 = WINDOW_ROW_Y1(instance->pane_size, y);

				name_icon->data.indirected_text_and_sprite.text = content.text;
				switch (content.status) {
				case WINDOW_STATUS_ERROR:
					name_icon->data.indirected_text_and_sprite.validation = "Serror";
					break;
				case WINDOW_STATUS_FAIL:
					name_icon->data.indirected_text_and_sprite.validation = "Sfail";
					break;
				case WINDOW_STATUS_PASS:
					name_icon->data.indirected_text_and_sprite.validation = "Spass";
					break;
				case WINDOW_STATUS_UNKNOWN:
					name_icon->data.indirected_text_and_sprite.validation = "Sunknown";
					break;
				}

				wimp_plot_icon(name_icon);
			}
		}

		more = wimp_get_rectangle(redraw);
	}
}


/**
 * Process data from a scroll event for a text window instance, updating
 * the window position in the associated data block as required and reopening
 * the window in the correct place.
 *
 * \param *instance		The instance to be scrolled.
 * \param *scroll		The scroll event data to be processed.
 */

static void window_scroll_handler(wimp_scroll *scroll)
{
	struct window_instance *instance = event_get_window_user_data(scroll->w);
	if (instance == NULL || instance->handle == NULL)
		return;

	int	width, height, error;

	/* Add in the X scroll offset. */

	width = scroll->visible.x1 - scroll->visible.x0;

	switch (scroll->xmin) {
	case wimp_SCROLL_COLUMN_LEFT:
		scroll->xscroll -= WINDOW_HORIZONTAL_SCROLL;
		break;

	case wimp_SCROLL_COLUMN_RIGHT:
		scroll->xscroll += WINDOW_HORIZONTAL_SCROLL;
		break;

	case wimp_SCROLL_PAGE_LEFT:
		scroll->xscroll -= width;
		break;

	case wimp_SCROLL_PAGE_RIGHT:
		scroll->xscroll += width;
		break;

	case wimp_SCROLL_AUTO_LEFT:
	case wimp_SCROLL_AUTO_RIGHT:
		/* We don't support Auto Scroll. */
		break;

	default: /* Extended Scroll */
		if (scroll->xmin < 0)
			scroll->xscroll -= (scroll->xmin >> 2) * WINDOW_HORIZONTAL_SCROLL;
		else if (scroll->xmin > 0)
			scroll->xscroll += (scroll->xmin >> 2) * WINDOW_HORIZONTAL_SCROLL;
		break;
 	}

	/* Add in the Y scroll offset. */

	height = (scroll->visible.y1 - scroll->visible.y0) - instance->pane_size;

	switch (scroll->ymin) {
	case wimp_SCROLL_LINE_UP:
		scroll->yscroll += WINDOW_ROW_HEIGHT;
		if ((error = ((scroll->yscroll) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= WINDOW_ROW_HEIGHT + error;
		break;

	case wimp_SCROLL_LINE_DOWN:
		scroll->yscroll -= WINDOW_ROW_HEIGHT;
		if ((error = ((scroll->yscroll - height) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= error;
		break;

	case wimp_SCROLL_PAGE_UP:
		scroll->yscroll += height;
		if ((error = ((scroll->yscroll) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= WINDOW_ROW_HEIGHT + error;
		break;

	case wimp_SCROLL_PAGE_DOWN:
		scroll->yscroll -= height;
		if ((error = ((scroll->yscroll - height) % WINDOW_ROW_HEIGHT)))
			scroll->yscroll -= error;
		break;

	case wimp_SCROLL_AUTO_UP:
	case wimp_SCROLL_AUTO_DOWN:
		/* We don't support Auto Scroll. */
		break;

	default: /* Extended Scroll */
		if (scroll->ymin > 0) {
			scroll->yscroll += (scroll->ymin >> 2) * height;
			if ((error = ((scroll->yscroll) % WINDOW_ROW_HEIGHT)))
				scroll->yscroll -= WINDOW_ROW_HEIGHT + error;
		} else if (scroll->ymin < 0) {
			scroll->yscroll -= (-scroll->ymin >> 2) * height;
			if ((error = ((scroll->yscroll - height) % WINDOW_ROW_HEIGHT)))
				scroll->yscroll -= error;
		}
		break;
	}

	wimp_open_window((wimp_open *) scroll);
}




#if 0
/**
 * Return the Wimp window handle for the window used by a
 * text window instance.
 *
 * \param *instance		The instance to be queried.
 * \return			The window handle.
 */

wimp_w window_get_handle(struct window_instance *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->handle;
}


/**
 * Return the Wimp window handle for the pane used by a
 * text window instance.
 *
 * \param *instance		The instance to be queried.
 * \return			The pane handle.
 */

wimp_w window_get_pane_handle(struct window_instance *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->pane_handle;
}


/**
 * Open a text window instance.
 *
 * \param *instance		The instance to be opened.
 */

void window_open(struct window_instance *instance)
{
	if (instance == NULL || instance->handle == NULL)
		return;

	windows_open(instance->handle);

	if (instance->pane_handle != NULL)
		windows_open_nested_as_toolbar(instance->pane_handle, instance->handle, instance->pane_size, FALSE);
}




/**
 * Process data for redraw events on a text window instance.
 *
 * \param instance		The text window instance to be redrawn.
 * \param *redraw		Pointer to the redraw data block.
 * \param *plotter		Pointer to a line plotter function.
 * \param *data			A data pointer to be passed to the plotter.
 */

void window_redraw(struct window_instance *instance, wimp_draw *redraw, osbool (*plotter)(int, struct window_redraw *, void *), void *data)
{
	int				top = 0, base = 0, ox = 0, oy = 0, y;
	struct window_redraw		line_info;
	char				buffer[WINDOW_LINE_BUFFER_LEN];
	os_coord			pos;
	osbool				more;

	/* Perform the redraw. */

	window_find_fonts();

	more = wimp_redraw_window(redraw);

	if (instance != NULL) {
		ox = redraw->box.x0 - redraw->xscroll;
		oy = redraw->box.y1 - redraw->yscroll;
	}

	pos.x = ox;

	while (more) {
		/* Calculate the top and bottom rows for redraw. */

		if (instance != NULL) {
			top = WINDOW_REDRAW_TOP(instance->pane_size, oy - redraw->clip.y1);
			if (top < 0)
				top = 0;

			base = WINDOW_REDRAW_BASE(instance->pane_size, oy - redraw->clip.y0);
		}

		/* Redraw the data into the window. */

		if (plotter != NULL) {
			for (y = top; y <= base; y++) {
				if (plotter(y, &line_info, data) == TRUE) {
					pos.y = oy + WINDOW_ROW_Y0(instance->pane_size, y);

					switch (line_info.type) {
					case WINDOW_TYPE_NONE:
						break;
					case WINDOW_TYPE_VALUE:
						window_format_numeric_data(&line_info, buffer, WINDOW_LINE_BUFFER_LEN);
						window_paint_text(&line_info, buffer, &pos);
						break;
					case WINDOW_TYPE_TEXT:
						window_paint_text(&line_info, line_info.text, &pos);
						break;
					}
				}
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	window_lose_fonts();
}


/**
 * Set the extent of a text block window.
 *
 * \param *instance		The instance to update.
 * \param lines			The number of lines to include.
 * */

void window_set_extent(struct window_instance *instance, int lines)
{
	wimp_window_state	state;
	os_box			extent;
	int			visible_extent, new_extent, new_scroll;

	if (instance == NULL || instance->handle == NULL)
		return;

	/* The new vertical extent. */

	if (lines < WINDOW_MINIMUM_SIZE)
		lines = WINDOW_MINIMUM_SIZE;

	new_extent = (-WINDOW_ROW_HEIGHT * lines) - instance->pane_size;

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	state.w = instance->handle;
	wimp_get_window_state(&state);

	visible_extent = state.yscroll + (state.visible.y0 - state.visible.y1);

	/* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

	if (new_extent > visible_extent) {
		new_scroll = new_extent - (state.visible.y0 - state.visible.y1);

		if (new_scroll > 0) {
			state.visible.y0 += new_scroll;
			state.yscroll = 0;
		} else {
			state.yscroll = new_scroll;
		}

		wimp_open_window((wimp_open *) &state);
	}

	/* Call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still exist. */

	extent.x0 = 0;
	extent.x1 = instance->width;
	extent.y0 = new_extent;
	extent.y1 = 0;

	wimp_set_extent(instance->handle, &extent);
}


/**
 * Format a piece of numeric data supplied as part of a redraw routine.
 *
 * \param *value		Pointer to the data to be formatted.
 * \param *buffer		Pointer to a buffer to take the result.
 * \param length		The length of the supplied buffer.
 */

static void window_format_numeric_data(struct window_redraw *value, char *buffer, size_t length)
{
	char c0 = ' ', c1 = ' ', c2 = ' ', c3 = ' ', *separator = "", *caption = "";

	if (buffer == NULL || length == 0)
		return;

	buffer[0] = '\0';

	if (value == NULL)
		return;

	c0 = value->value & 0xff;
	if (c0 < 32 || c0 >= 127)
		c0 = '.';

	if (value->bytes > 1) {
		c1 = (value->value >> 8) & 0xff;
		if (c1 < 32 || c1 >= 127)
			c1 = '.';
	}

	if (value->bytes > 2) {
		c2 = (value->value >> 16) & 0xff;
		if (c2 < 32 || c2 >= 127)
			c2 = '.';
	}

	if (value->bytes > 3) {
		c3 = (value->value >> 24) & 0xff;
		if (c3 < 32 || c3 >= 127)
			c3 = '.';
	}

	if (value->text != NULL) {
		separator = " <- ";
		caption = value->text;
	}

	snprintf(buffer, length, "%10d : %08x : %c%c%c%c : %-10d%s%s", value->index,
			value->value, c0, c1, c2, c3, value->value, separator, caption);
}


/**
 * Find the fonts required to plot into a window.
 *
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *window_find_fonts(void)
{
	os_error *error = NULL;
	int size = 192; // 12 pt

	if (window_normal_font == 0 && error == NULL) {
		error = xfont_find_font("Corpus.Medium", size, size, 0, 0, &window_normal_font, NULL, NULL);
		if (error != NULL)
			window_normal_font = font_SYSTEM;
	}

	if (window_bold_font == 0 && error == NULL) {
		error = xfont_find_font("Corpus.Bold", size, size, 0, 0, &window_bold_font, NULL, NULL);
		if (error != NULL)
			window_bold_font = font_SYSTEM;
	}

	return error;
}


/**
 * Lose the fonts used to plot into a window.
 */

static void window_lose_fonts(void)
{
	if (window_normal_font != 0)
		font_lose_font(window_normal_font);

	if (window_bold_font != 0)
		font_lose_font(window_bold_font);

	window_normal_font = font_SYSTEM;
	window_bold_font = font_SYSTEM;
}


/**
 * Paint a line into a window.
 *
 * \param *line_info		Pointer to the line details.
 * \param *text			Pointer to an alternative text line, when required.
 * \param *pos			Pointer to a coordinate block.
 * \return			Pointer to an error block, or NULL if successful.
 */

static os_error *window_paint_text(struct window_redraw *line_info, char *text, os_coord *pos)
{
	os_error *error;
	font_f font;

	if (line_info == NULL)
		return NULL;

	font = (line_info->bold == TRUE) ? window_bold_font : window_normal_font;

	if (text == NULL || font == font_SYSTEM)
		return NULL;

	error = xcolourtrans_set_font_colours(font, os_COLOUR_VERY_LIGHT_GREY, line_info->colour, 14, NULL, NULL, NULL);
	if (error != NULL)
		return error;

	return xfont_paint(font, text, font_OS_UNITS | font_KERN | font_GIVEN_FONT, pos->x, pos->y, NULL, NULL, 0);
}

#endif