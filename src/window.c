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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

#include <flex.h>

/* OSLib header files */

#include <oslib/colourtrans.h>
#include <oslib/font.h>
#include <oslib/os.h>
#include <oslib/osspriteop.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/event.h>
#include <sflib/heap.h>
#include <sflib/icons.h>
#include <sflib/string.h>
#include <sflib/templates.h>
#include <sflib/windows.h>

/* Application header files */

#include "window.h"

#include "date_time.h"
#include "flexutils.h"

/* Constant definitions. */

/**
 * The size of a line text buffer.
 */

#define WINDOW_LINE_BUFFER_LEN 256

/**
 * The minimum number of lines to show in a window.
 */

#define WINDOW_MINIMUM_SIZE 20

/**
 * The minimum width of a window content, in OS units.
 */

#define WINDOW_MINIMUM_CONTENT_WIDTH 400

/**
 * The size of the buffer used for plotting the detail icon contents.
 */

#define WINDOW_DETAIL_BUFFER_LEN 16

/**
 * The amount of space allocated for the toolbar date field.
 */

#define WINDOW_DATE_FIELD_LEN 32

/**
 * The size of a horizontal scroll step.
 */

#define WINDOW_HORIZONTAL_SCROLL 16

/**
 * The height of a row icon in a window table.
 */

#define WINDOW_ROW_ICON_HEIGHT 36

/**
 * The horizontal spacing between rows in a window table.
 */

#define WINDOW_ROW_GUTTER 4

/**
 * The width of an expand icon.
 */

#define WINDOW_EXPAND_WIDTH 36

/**
 * The distance to indent lines in the window.
 */

#define WINDOW_INDENT_WIDTH 28

/**
 * The height of a window row.
 */

#define WINDOW_ROW_HEIGHT (WINDOW_ROW_ICON_HEIGHT + WINDOW_ROW_GUTTER)

/**
 * The icon templates.
 */

#define WINDOW_TEMPLATE_ICON_EXPAND 0
#define WINDOW_TEMPLATE_ICON_NAME 1
#define WINDOW_TEMPLATE_ICON_DETAIL 2

/**
 * The toolbar icons.
 */

#define WINDOW_TOOLBAR_ICON_DATE 0
#define WINDOW_TOOLBAR_ICON_BACK 1
#define WINDOW_TOOLBAR_ICON_FORWARD 2
#define WINDOW_TOOLBAR_ICON_LATEST 3
#define WINDOW_TOOLBAR_ICON_RUN 4

#define WINDOW_ALLOCATION_UNIT 10

#define WINDOW_LINE_NONE ((unsigned) 0xffffffffu)

/* Structure definitions. */

/**
 * A window fold object.
 */

struct window_object {
	unsigned line;
	osbool expanded;
};

struct window_fold {
	unsigned object;
	int entries;
};

/**
 * A Text Window Instance Definition.
 */

struct window_instance {
	struct window_definition *definition;	/**< The window defintion.			*/
	void *client_data;			/**< The client data pointer.			*/

	wimp_w handle;				/**< The Wimp handle of the window.		*/
	wimp_w pane_handle;			/**< The Wimp handle of the pane.		*/
	int pane_size;				/**< The height of a toolbar pane, in OS units.	*/
	int width;				/**< The window width in OS units.		*/

	int display_lines;			/**< The number of items in the window.		*/


	char date_field[WINDOW_DATE_FIELD_LEN];	/**< Storage for the date field icon.		*/

	struct window_object *known_objects;	/**< Array of window objects.			*/
	int object_space;
	int object_count;

	struct window_fold *active_folds;
	int fold_space;
	int fold_count;
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

static void window_open_handler(wimp_open *open);
static void window_close_handler(wimp_close *close);
static void window_click_handler(wimp_pointer *pointer);
static void window_redraw_handler(wimp_draw *redraw);
static void window_scroll_handler(wimp_scroll *scroll);
static osbool window_recalculate_columns(struct window_instance *instance, wimp_open *open);
static void window_position_toolbar_icons(wimp_open *open, struct window_instance *instance);
static void window_toggle_fold_icon(struct window_instance *instance, int fold);
static void window_recalculate_display_lines(struct window_instance *instance);
static void window_set_extent(struct window_instance *instance);
static void window_force_redraw_fold(struct window_instance *instance, int fold);
static void window_force_redraw_fold_to_end(struct window_instance *instance, int fold);
static void window_force_redraw_lines(struct window_instance *instance, int first, int last);
static osbool window_decode_click_data(struct window_instance *instance, wimp_pointer *pointer, int *fold, int *entry, wimp_i *icon);
static osbool window_get_rows_from_fold(struct window_instance *instance, int fold, int *top, int *bottom);
static osbool window_get_fold_info_from_row(struct window_instance *instance, int row, int *fold_out, int *entry_out);

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
	window_pane_definition->sprite_area = sprites;
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

	instance->known_objects = NULL;
	instance->object_space = WINDOW_ALLOCATION_UNIT;
	instance->object_count = 0;

	instance->active_folds = NULL;
	instance->fold_space = WINDOW_ALLOCATION_UNIT;
	instance->fold_count = 0;

	instance->display_lines = 0;

	/* Allocate the flex blocks. */

	if (!flexutils_allocate((void **) &(instance->known_objects), sizeof(struct window_object *), instance->object_space)) {
		window_delete_instance(instance);
		return NULL;
	}

	if (!flexutils_allocate((void **) &(instance->active_folds), sizeof(struct window_fold *), instance->fold_space)) {
		window_delete_instance(instance);
		return NULL;
	}

	/* Create the new window. */

	os_error *error = xwimp_create_window(window_definition, &(instance->handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		window_delete_instance(instance);
		return NULL;
	}

	/* Create the new window pane. */

	instance->pane_size = window_pane_definition->visible.y1 - window_pane_definition->visible.y0;

	instance->date_field[0] = '\0';
	window_pane_definition->icons[WINDOW_TOOLBAR_ICON_DATE].data.indirected_text.text = instance->date_field;
	window_pane_definition->icons[WINDOW_TOOLBAR_ICON_DATE].data.indirected_text.size = WINDOW_DATE_FIELD_LEN;

	windows_place_as_toolbar(window_definition, window_pane_definition, instance->pane_size);

	error = xwimp_create_window(window_pane_definition, &(instance->pane_handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		window_delete_instance(instance);
		return NULL;
	}

	event_add_window_user_data(instance->handle, instance);
	event_add_window_open_event(instance->handle, window_open_handler);
	event_add_window_close_event(instance->handle, window_close_handler);
	event_add_window_redraw_event(instance->handle, window_redraw_handler);
	event_add_window_mouse_event(instance->handle, window_click_handler);
	event_add_window_scroll_event(instance->handle, window_scroll_handler);

	event_add_window_user_data(instance->pane_handle, instance);
//	event_add_window_mouse_event(instance->pane_handle, window_click_handler);

	/* Open the windows. */

	wimp_window_state window;

	window.w = instance->handle;
	wimp_get_window_state(&window);
	window_recalculate_columns(instance, (wimp_open *) &window);
	window.next = wimp_TOP;
	wimp_open_window((wimp_open *) &window);

	windows_open_nested_as_toolbar(instance->pane_handle, instance->handle, instance->pane_size, FALSE);
	window_position_toolbar_icons((wimp_open *) &window, instance);

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

	flexutils_free((void **) &(instance->known_objects));
	flexutils_free((void **) &(instance->active_folds));

	heap_free(instance);
}

/**
 * Handle Open events on an instance window.
 *
 * \param *open			The Wimp Open data block.
 */

static void window_open_handler(wimp_open *open)
{
	struct window_instance *instance = event_get_window_user_data(open->w);
	if (instance == NULL || open->w != instance->handle)
		return;

	if (window_recalculate_columns(instance, open)) {
		windows_redraw(open->w);
		window_position_toolbar_icons(open, instance);
	}

	wimp_open_window(open);
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

/**
 * Handle Mouse events on an instance window.
 *
 * \param *pointer		The Wimp Pointer data block.
 */

static void window_click_handler(wimp_pointer *pointer)
{
	struct window_instance *instance = event_get_window_user_data(pointer->w);
	if (instance == NULL)
		return;

	int fold = 0, entry = 0;
	wimp_i icon = wimp_ICON_WINDOW;

	if (!window_decode_click_data(instance, pointer, &fold, &entry, &icon))
		return;

	switch (icon) {
	case WINDOW_TEMPLATE_ICON_EXPAND:
		window_toggle_fold_icon(instance, fold);
		break;
	}
}

/**
 * Handle Redraw events on an instance window.
 *
 * \param *redraw		The Wimp Redraw data block.
 */

static void window_redraw_handler(wimp_draw *redraw)
{
	struct window_instance *instance = event_get_window_user_data(redraw->w);

	wimp_icon *expand_icon = window_definition->icons + WINDOW_TEMPLATE_ICON_EXPAND;
	wimp_icon *name_icon = window_definition->icons + WINDOW_TEMPLATE_ICON_NAME;
	wimp_icon *detail_icon = window_definition->icons + WINDOW_TEMPLATE_ICON_DETAIL;

	int detail_column_width = detail_icon->extent.x1 - detail_icon->extent.x0;

	char detail_buffer[WINDOW_DETAIL_BUFFER_LEN];

	detail_icon->extent.x1 = instance->width - WINDOW_ROW_GUTTER;
	detail_icon->extent.x0 = detail_icon->extent.x1 - detail_column_width;
	detail_icon->data.indirected_text.text = detail_buffer;
	detail_icon->data.indirected_text.size = WINDOW_DETAIL_BUFFER_LEN;

	expand_icon->extent.x0 = WINDOW_ROW_GUTTER;
	expand_icon->extent.x1 = expand_icon->extent.x0 + WINDOW_EXPAND_WIDTH;

	int name_icon_base_x0 = expand_icon->extent.x1 + WINDOW_ROW_GUTTER;
	name_icon->extent.x1 = detail_icon->extent.x0 - WINDOW_ROW_GUTTER;

	/* Perform the redraw. */

	osbool more = wimp_redraw_window(redraw);

	/* Work out the redraw origin. */

	int oy = (instance != NULL) ? redraw->box.y1 - redraw->yscroll : 0;

	while (more) {
		if (instance != NULL && instance->definition->callback_redraw != NULL) {
			int top = ((oy - redraw->clip.y1) - instance->pane_size) / WINDOW_ROW_HEIGHT;
	//		int top = WINDOW_REDRAW_TOP(instance->pane_size, oy - redraw->clip.y1);
			if (top < 0)
				top = 0;

			int base = ((oy - redraw->clip.y0) - instance->pane_size) / WINDOW_ROW_HEIGHT;
	//		int base = WINDOW_REDRAW_BASE(instance->pane_size, oy - redraw->clip.y0);
			if (base >= instance->display_lines)
				base = instance->display_lines - 1;

			int fold = 0, entry = 0;

			if (window_get_fold_info_from_row(instance, top, &fold, &entry)) {
				for (int y = top; y <= base; y++) {
					if (fold >= instance->fold_count)
						break;

					/* Request the line details from the client. */

					struct window_line content;

					if (instance->definition->callback_redraw(fold, entry, &content, instance->client_data) == FALSE)
						break;

					/* Set up the vertical positions of the icons. */

					detail_icon->extent.y1 = -((y * WINDOW_ROW_HEIGHT) + WINDOW_ROW_GUTTER + instance->pane_size);
					detail_icon->extent.y0 = detail_icon->extent.y1 - WINDOW_ROW_ICON_HEIGHT;

					name_icon->extent.y1 = detail_icon->extent.y1;
					name_icon->extent.y0 = detail_icon->extent.y0;

					expand_icon->extent.y1 = detail_icon->extent.y1;
					expand_icon->extent.y0 = detail_icon->extent.y0;

					/* Fill in the icons from the client's data. */

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

					string_printf(detail_buffer, WINDOW_DETAIL_BUFFER_LEN, "%d/%d", content.count, content.total);

					/* Vary the line layout depending on whether or not this is fold. */

					struct window_fold *f = instance->active_folds + fold;
					struct window_object *o = instance->known_objects + f->object;

					if (entry < 0) {
						if (f->entries > 0) {
							string_copy(expand_icon->data.sprite, (o->expanded) ? "expand" : "contract", 12);
							expand_icon->flags &= ~wimp_ICON_SHADED;
						} else {
							string_copy(expand_icon->data.sprite, "contract", 12);
							expand_icon->flags |= wimp_ICON_SHADED;
						}

						wimp_plot_icon(expand_icon);
						wimp_plot_icon(detail_icon);

						name_icon->extent.x0 = name_icon_base_x0;
					} else {
						name_icon->extent.x0 = name_icon_base_x0 + WINDOW_INDENT_WIDTH;
					}

					wimp_plot_icon(name_icon);

					/* Step on to the next line. */

					if (entry < 0 && o->expanded == FALSE) {
						fold += 1;
					} else {
						entry += 1;
						if (entry >= f->entries) {
							entry = -1;
							fold += 1;
						}
					}
				}
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

	/* Add in the X scroll offset. */

	int width = scroll->visible.x1 - scroll->visible.x0;

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

	int height = (scroll->visible.y1 - scroll->visible.y0) - instance->pane_size;
	int error = 0;

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


/**
 * Recalculate the columns of an instance window.
 *
 * \param *instance		The instance to be recalculated.
 * \param *open			The Wimp Open data block.
 * \return			TRUE if the window will need to be redrawn.
 */

static osbool window_recalculate_columns(struct window_instance *instance, wimp_open *open)
{
	if (instance == NULL || open == NULL)
		return FALSE;

	int new_width = open->visible.x1 - open->visible.x0;

	if (new_width < WINDOW_MINIMUM_CONTENT_WIDTH)
		new_width = WINDOW_MINIMUM_CONTENT_WIDTH;

	if (new_width == instance->width)
		return FALSE;

	instance->width = new_width;

	return TRUE;
}

/**
 * Update the positions of the toolbar icons.
 *
 * \param *open			The Wimp Open data block which triggered the
 *				update.
 * \param *instance		The window instance to be updated.
 */

static void window_position_toolbar_icons(wimp_open *open, struct window_instance *instance)
{
	wimp_icon_state icon_state = { .w = instance->pane_handle, .i = WINDOW_TOOLBAR_ICON_DATE };

	os_error *error = xwimp_get_icon_state(&icon_state);
	if (error != NULL)
		return;

	int window_rhs = (open->visible.x1 - open->visible.x0) + open->xscroll;

	error = xwimp_resize_icon(
		icon_state.w,
		icon_state.i,
		icon_state.icon.extent.x0,
		icon_state.icon.extent.y0,
		window_rhs - 4,
		icon_state.icon.extent.y1
	);
	if (error != NULL)
		return;

	xwimp_force_redraw(
		icon_state.w,
		icon_state.icon.extent.x0,
		icon_state.icon.extent.y0,
		window_rhs,
		icon_state.icon.extent.y1
	);
}

/**
 * Start to rebuild the contents of a window, discarding any previous content
 * and setting up the timestamp.
 *
 * After calling this function, window_add_new_fold() may be called a number
 * of times to add folds. Once complete, window_finish_new_content() must be
 * called to finish off the window calculations.
 *
 * \param *instance		Pointer to the window instance to be updated.
 * \param time			The timestamp of the new content.
 */

void window_start_new_content(struct window_instance *instance, uint64_t time)
{
	if (instance == NULL)
		return;

	debug_printf("Start new window content.");

	/* Reset any existing folds. */

	instance->fold_count = 0;

	for (int i = 0; i < instance->object_count; i++)
		instance->known_objects[i].line = WINDOW_NULL_FOLD;

	/* Update the date field. */

	date_time_write_standard_string(time, instance->date_field, WINDOW_DATE_FIELD_LEN);
	wimp_set_icon_state(instance->pane_handle, WINDOW_TOOLBAR_ICON_DATE, 0, 0);
}

/**
 * Add a new fold to a window as part of a content update.
 *
 * Before calling this function, window_start_new_content() must have been
 * called. After calling it, window_finish_new_content() must be called.
 *
 * \param *instance		Pointer to the window instance being updated.
 * \param id			A window object ID for the fold contents, if
 *				one has previously be allocated.
 * \param entries		The number of entries to be contained in the
 *				fold.
 * \return			A window object ID for the fold contents, which
 *				may be the one supplied in the id parameter.
 */

unsigned window_add_new_fold(struct window_instance *instance, unsigned id, int entries)
{
	if (instance == NULL)
		return WINDOW_NULL_FOLD;

	/* Claim an object from the store if the client didn't provide one. */

	if (id == WINDOW_NULL_FOLD) {
		debug_printf("Claiming new known object...");

		if (instance->object_count >= instance->object_space) {
			debug_printf("Known object space full... expanding...");

			size_t new_space = instance->object_space;

			while (new_space <= instance->object_count)
				new_space += WINDOW_ALLOCATION_UNIT;

			if (flexutils_resize((void **) &(instance->known_objects), sizeof(struct window_object *), new_space))
				instance->object_space = new_space;
		}

		if (instance->object_count >= instance->object_space)
			return WINDOW_NULL_FOLD;

		id = instance->object_count;
		instance->object_count++;

		instance->known_objects[id].expanded = FALSE;
		instance->known_objects[id].line = WINDOW_NULL_FOLD;

	}

	/* The id is out of range. */

	if (id >= instance->object_count)
		return WINDOW_NULL_FOLD;

	debug_printf("Known object id=%u", id);

	/* Add a fold to the window. */

	if (instance->fold_count >= instance->fold_count) {
		debug_printf("Active fold space full... expanding...");

		size_t new_space = instance->fold_space;

		while (new_space <= instance->fold_count)
			new_space += WINDOW_ALLOCATION_UNIT;

		if (flexutils_resize((void **) &(instance->active_folds), sizeof(struct window_fold *), new_space))
			instance->fold_space = new_space;
	}

	if (instance->fold_count >= instance->fold_space)
		return WINDOW_NULL_FOLD;

	int fold = instance->fold_count++;

	debug_printf("Added fold %d to window.", fold);

	instance->active_folds[fold].object = id;
	instance->active_folds[fold].entries = entries;

	instance->known_objects[id].line = fold;

	return id;
}

/**
 * Finish rebuilding the contents of a window, recalculating the content
 * details and redrawing the contents.
 *
 * This should be called after calling window_start_new_content().
 *
 * \param *instance		Pointer to the window instance being updated.
 */

void window_finish_new_content(struct window_instance *instance)
{
	if (instance == NULL)
		return;

	int entries = 0;

	window_recalculate_display_lines(instance);
	window_set_extent(instance);

	debug_printf("Finish new window content; extent = %d entries.", entries);
}

/**
 * Toggle the state of a fold within a window.
 *
 * \param *instance		Pointer to the window instance containing the
 *				fold to be toggled.
 * \param fold			The fold to be toggled.
 */

static void window_toggle_fold_icon(struct window_instance *instance, int fold)
{
	if (instance == NULL || fold < 0 || fold >= instance->fold_count)
		return;

	struct window_fold *f = instance->active_folds + fold;
	struct window_object *o = instance->known_objects + f->object;

	o->expanded = !o->expanded;

	window_recalculate_display_lines(instance);
	window_set_extent(instance);
	window_force_redraw_fold_to_end(instance, fold);
}

/**
 * Recalculate the number of display lines in a window, based on the content
 * and the state of the folds.
 *
 * \param *instance		Pointer to the window instance to be recalculated
 */

static void window_recalculate_display_lines(struct window_instance *instance)
{
	if (instance == NULL)
		return;

	instance->display_lines = 0;

	for (int i = 0; i < instance->fold_count; i++) {
		instance->display_lines += 1;

		struct window_fold *f = instance->active_folds + i;
		struct window_object *o = instance->known_objects + f->object;

		if (o->expanded == TRUE)
			instance->display_lines += f->entries;
	}
}

/**
 * Set the size of an in an instance window in terms of the number of entries
 * that it contains.
 *
 * \param *instance		The instance to update.
 * \param entries		The number of entries to show in the window.
 * */

static void window_set_extent(struct window_instance *instance)
{
	if (instance == NULL || instance->handle == NULL)
		return;

	int entries = instance->display_lines;

	/* The new vertical extent. */

	if (entries < WINDOW_MINIMUM_SIZE)
		entries = WINDOW_MINIMUM_SIZE;

	int new_extent = -((WINDOW_ROW_HEIGHT * entries) + instance->pane_size + WINDOW_ROW_GUTTER);

	/* Get the current window details, and find the extent of the bottom of the visible area. */

	wimp_window_state state;

	state.w = instance->handle;
	wimp_get_window_state(&state);

	int visible_extent = state.yscroll + (state.visible.y0 - state.visible.y1);

	/* If the visible area falls outside the new window extent, then the window needs to be re-opened first. */

	if (new_extent > visible_extent) {
		int new_scroll = new_extent - (state.visible.y0 - state.visible.y1);

		if (new_scroll > 0) {
			state.visible.y0 += new_scroll;
			state.yscroll = 0;
		} else {
			state.yscroll = new_scroll;
		}

		wimp_open_window((wimp_open *) &state);
	}

	/* Call Wimp_SetExtent to update the extent, safe in the knowledge that the visible area will still exist. */

	os_box extent;

	extent.x0 = window_definition->extent.x0;
	extent.x1 = window_definition->extent.x1;
	extent.y0 = new_extent;
	extent.y1 = window_definition->extent.y1;

	wimp_set_extent(instance->handle, &extent);
}

/**
 * Force the redraw of the lines relating to a fold. This includes the entries
 * if the fold is expanded.
 *
 * \param *instance		Pointer to the window instance to be redrawn.
 * \param fold			The fold to be redrawn.
 */

static void window_force_redraw_fold(struct window_instance *instance, int fold)
{
	if (instance == NULL)
		return;

	int top = 0, bottom = 0;

	if (!window_get_rows_from_fold(instance, fold, &top, &bottom))
		return;

	window_force_redraw_lines(instance, top, bottom);
}

/**
 * Force the redraw of the lines from a fold to the end of the window, including
 * any expanded entries.
 *
 * \param *instance		Pointer to the window instance to be redrawn.
 * \param fold			The fold from which to redraw.
 */

static void window_force_redraw_fold_to_end(struct window_instance *instance, int fold)
{
	if (instance == NULL)
		return;

	int top = 0;

	if (!window_get_rows_from_fold(instance, fold, &top, NULL))
		return;

	window_force_redraw_lines(instance, top, -1);
}

/**
 * Force the redraw of a range of lines within a window.
 *
 * \param *instance		Pointer to the window instance to be redrawn.
 * \param first			The first line to be redrawn.
 * \param last			The last line to be redrawn, or -1 to redraw to
 *				the end of the window extent.
 */

static void window_force_redraw_lines(struct window_instance *instance, int first, int last)
{
	if (instance == NULL || instance->handle == NULL)
		return;

	osbool redraw_to_last = (last == -1) ? TRUE : FALSE;

	if (redraw_to_last)
		last = instance->display_lines - 1;

	if (first < 0 || last < 0 || first > last)
		return;

	int top = -((first * WINDOW_ROW_HEIGHT) + WINDOW_ROW_GUTTER + instance->pane_size);
	int bottom = -(((last + 1) * WINDOW_ROW_HEIGHT) + WINDOW_ROW_GUTTER + instance->pane_size);

	if (redraw_to_last) {
		wimp_window_state state = { .w = instance->handle };
		wimp_get_window_state(&state);

		int visible = state.yscroll - (state.visible.y1 - state.visible.y0);
		if (visible < bottom)
			bottom = visible;
	}

	wimp_force_redraw(instance->handle, 0, bottom, instance->width, top);
}

/**
 * Decode wimp_pointer data into fold, entry and icon values.
 *
 * \param *instance		Pointer to the window instance to which the data
 *				relates.
 * \param *pointer		Pointer to the pointer data to be decoded.
 * \param *fold			Pointer to a valiable in which to return the
 *				calculated fold index, or NULL.
 * \param *entry		Pointer to a valiable in which to return the
 *				calculated entry index, or NULL.
 * \param *icon			Pointer to a valiable in which to return the
 *				calculated icon handle, or NULL.
 * \return			TRUE if the data could be decoded into a valid line;
 *				otherwise FALSE.
 */

static osbool window_decode_click_data(struct window_instance *instance, wimp_pointer *pointer, int *fold, int *entry, wimp_i *icon)
{
	if (instance == NULL || pointer == NULL)
		return FALSE;

	if (fold != NULL)
		*fold = 0;

	if (entry != NULL)
		*entry = 0;

	if (icon != NULL)
		*icon = wimp_ICON_WINDOW;

	/* Calculate the window X and Y coordinates. */

	wimp_window_state state = { .w = pointer->w };
	wimp_get_window_state(&state);

	int xpos = (pointer->pos.x - state.visible.x0) + state.xscroll;
	int ypos = (pointer->pos.y - state.visible.y1) + state.yscroll;

	/* Calculate the row. */

	int row = -(ypos + instance->pane_size + 1) / WINDOW_ROW_HEIGHT;

	if (row >= instance->display_lines)
		return FALSE;

	/* Calculate the vertical position in the row. */

	int vpos = -(ypos + instance->pane_size + 1) % WINDOW_ROW_HEIGHT;

	if (vpos < WINDOW_ROW_GUTTER || vpos >= (WINDOW_ROW_GUTTER + WINDOW_ROW_HEIGHT))
		return FALSE;

	/* Decode the fold and entry details. */

	int our_fold = 0, our_entry = 0;

	if (fold == NULL)
		fold = &our_fold;

	if (entry == NULL)
		entry = &our_entry;

	if (!window_get_fold_info_from_row(instance, row, fold, entry))
		return FALSE;

	/* Identify the icon position within the row. */

	if (icon == NULL)
		return TRUE;

	wimp_icon *detail_icon = window_definition->icons + WINDOW_TEMPLATE_ICON_DETAIL;
	int detail_column_width = detail_icon->extent.x1 - detail_icon->extent.x0;

	if (*entry == -1 && xpos >= WINDOW_ROW_GUTTER &&
			xpos < (WINDOW_ROW_GUTTER + WINDOW_EXPAND_WIDTH))
		*icon = WINDOW_TEMPLATE_ICON_EXPAND;

	else if (*entry == -1 && xpos >= (instance->width - (WINDOW_ROW_GUTTER + detail_column_width)) &&
			xpos < (instance->width - WINDOW_ROW_GUTTER))
		*icon = WINDOW_TEMPLATE_ICON_DETAIL;

	else if (*entry == -1 && xpos >= ((2 * WINDOW_ROW_GUTTER) + WINDOW_EXPAND_WIDTH) &&
			xpos < ((instance->width - ((2 * WINDOW_ROW_GUTTER) + detail_column_width))))
		*icon = WINDOW_TEMPLATE_ICON_NAME;

	else if (*entry >= 0 && xpos >= ((2 * WINDOW_ROW_GUTTER) + WINDOW_EXPAND_WIDTH + WINDOW_INDENT_WIDTH) &&
			xpos < ((instance->width - ((2 * WINDOW_ROW_GUTTER) + detail_column_width))))
		*icon = WINDOW_TEMPLATE_ICON_NAME;

	return TRUE;
}

/**
 * Given a fold in a window instance, work out the corresponding display row
 * range.
 *
 * On exit, top and bottom may be equal if the fold is contracted.
 *
 * \param *instance		Pointer to the window instance containing the fold.
 * \param fold			The fold to be decoded.
 * \param *top			Pointer to a variable to take the returned top row
 *				number, or NULL.
 * \param *bottom		Pointer to a variable to take the returned bottom
 *				row number, or NULL.
 * \return			TRUE if the fold could be decoded; FALSE on failure.
 */

static osbool window_get_rows_from_fold(struct window_instance *instance, int fold, int *top, int *bottom)
{
	if (instance == NULL || fold < 0 || fold >= instance->fold_count)
		return FALSE;

	int row = 0;

	for (int i = 0; i < fold; i++) {
		struct window_fold *f = instance->active_folds + i;
		struct window_object *o = instance->known_objects + f->object;

		/* The fold line. */

		row += 1;

		if (o->expanded == TRUE)
			row += f->entries;
	}

	struct window_fold *f = instance->active_folds + fold;
	struct window_object *o = instance->known_objects + f->object;

	if (top != NULL)
		*top = row;

	if (bottom != NULL)
		*bottom = row + (o->expanded == TRUE) ? f->entries : 0;

	return TRUE;
}

/**
 * Given a row in a window instance, work out the fold and entry information.
 *
 * \param *instance		Pointer to the window instance containing the row.
 * \param row			The row to be decoded.
 * \param *fold			Pointer to a variable to take the returned fold
 *				number, or NULL.
 * \param *entry		Pointer to a variable to take the returned entry
 *				number, or NULL.
 * \return			TRUE if the row could be decoded; FALSE on failure.
 */

static osbool window_get_fold_info_from_row(struct window_instance *instance, int row, int *fold, int *entry)
{
	if (instance == NULL || row < 0)
		return FALSE;

	for (int i = 0; i < instance->fold_count; i++) {
		if (row == 0) {
			/* We're out of rows, so this must be the line. */

			if (fold != NULL)
				*fold = i;
			if (entry != NULL)
				*entry = -1;

			return TRUE;
		}

		/* Remove the fold line and find the entry details. */

		row--;

		struct window_fold *f = instance->active_folds + i;
		struct window_object *o = instance->known_objects + f->object;

		/* If the fold isn't expanded, continue. */

		if (o->expanded == FALSE)
			continue;

		/* The fold is expanded, so check the entries within it. */

		if (row < f->entries) {
			if (fold != NULL)
				*fold = i;

			if (entry != NULL)
				*entry = row;

			return TRUE;
		}

		row -= f->entries;
	}

	return FALSE;
}


#if 0

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