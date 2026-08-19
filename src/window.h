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
 * \file: window.h
 *
 * Text Window interface.
 */

#ifndef UNIFY_WINDOW
#define UNIFY_WINDOW

#include "oslib/os.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/**
 * A text window instance.
 */

struct window_instance;

/**
 * The type of window.
 */

enum window_type {
	WINDOW_TYPE_NONE,
	WINDOW_TYPE_SUITE
};

/**
 * The line statuses for the window entries.
 */

enum window_status {
	WINDOW_STATUS_UNKNOWN,
	WINDOW_STATUS_ERROR,
	WINDOW_STATUS_FAIL,
	WINDOW_STATUS_PASS
};

/**
 * Data for a window line redraw.
 */

struct window_line {
	enum window_status status;		/**< The entry status.			*/
	char *text;				/**< The text for the line.		*/
	int count;				/**< The line count, for suite entries.	*/
	int total;				/**< The line total, for suite entries.	*/
};

/**
 * A client definition for a window instance
 */

struct window_definition {
	enum window_type type;			/**< The type of window.		*/

	/**
	 * Callack when the window closes.
	 */
	void (*callback_close)(void *data);

	/**
	 * Callback for requesting line redraw data.
	 */
	osbool (*callback_redraw)(int line, struct window_line *content, void *data);
};

#if 0
/**
 * A line redraw data block.
 */

struct window_redraw {
	enum window_type type;		/**< The type of data on the line.		*/
	os_colour colour;		/**< The colour of the text.			*/
	osbool bold;			/**< Is the text bold?				*/
	char *text;			/**< Pointer to the line text.			*/
	unsigned value;			/**< The value for a value line.		*/
	int index;			/**< The index for a value line.		*/
	int bytes;			/**< The number of bytes for a value line.	*/
};
#endif

/**
 * The size of a horizontal scroll step.
 */

#define WINDOW_HORIZONTAL_SCROLL 16

/**
 * The height of a row icon in a window table.
 */

#define WINDOW_ROW_ICON_HEIGHT 52

/**
 * The horizontal spacing between rows in a window table.
 */

#define WINDOW_ROW_GUTTER 4

/**
 * The height of a window row.
 */

#define WINDOW_ROW_HEIGHT (WINDOW_ROW_ICON_HEIGHT + WINDOW_ROW_GUTTER)

/**
 * Calculate the first row to be included in a redraw operation.
 */

#define WINDOW_REDRAW_TOP(toolbar, y) (((y) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Calculate the last row to be included in a redraw operation.
 */

#define WINDOW_REDRAW_BASE(toolbar, y) (((y) - (toolbar) - 2) / WINDOW_ROW_HEIGHT)

/**
 * Calculate the base of a row in a table view.
 */

#define WINDOW_ROW_BASE(toolbar, y) ((-((y) + 1) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the top of a row in a table view.
 */

#define WINDOW_ROW_TOP(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar) + WINDOW_ROW_GUTTER)

/**
 * Calculate the base of an icon in a table view.
 */

#define WINDOW_ROW_Y0(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar) - WINDOW_ROW_ICON_HEIGHT)

/**
 * Calculate the top of an icon in a table view.
 */

#define WINDOW_ROW_Y1(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the raw row number based on a window mouse coordinate.
 */

#define WINDOW_ROW(toolbar, y) (((-(y)) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Caluclate the position within a row, given a window mouse coordinate.
 */

#define WINDOW_ROW_Y_POS(toolbar, y) (((-(y)) - (toolbar)) % WINDOW_ROW_HEIGHT)

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

#define WINDOW_ROW_BELOW(y) ((y) < WINDOW_ROW_GUTTER)
#define WINDOW_ROW_ABOVE(y) ((y) > WINDOW_ROW_HEIGHT)

/**
 * Initialise the text window.
 *
 * \param *sprites		Pointet to the user sprite area.
 */

void window_initialise(osspriteop_area *sprites);

/**
 * Create a new window instance.
 *
 * \param *pane_definition	Pointer to the window definition.
 * \param *client_data		Pointer to the client data, or NULL for none.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct window_instance *window_create_instance(struct window_definition *definition, void *client_data);
/**
 * Destroy a text window instance.
 *
 * \param *instance		The instance to be deleted.
 */

void window_delete_instance(struct window_instance *instance);
#if 0
/**
 * Return the Wimp window handle for the window used by a
 * text window instance.
 *
 * \param *instance		The instance to be queried.
 * \return			The window handle.
 */

wimp_w window_get_handle(struct window_instance *instance);

/**
 * Return the Wimp window handle for the pane used by a
 * text window instance.
 *
 * \param *instance		The instance to be queried.
 * \return			The pane handle.
 */

wimp_w window_get_pane_handle(struct window_instance *instance);

/**
 * Open a text window instance.
 *
 * \param *instance		The instance to be opened.
 */

void window_open(struct window_instance *instance);

/**
 * Process data from a scroll event for a text window instance, updating
 * the window position in the associated data block as required and reopening
 * the window in the correct place.
 *
 * \param *instance		The instance to be scrolled.
 * \param *scroll		The scroll event data to be processed.
 */

void window_process_scroll_event(struct window_instance *instance, wimp_scroll *scroll);


/**
 * Process data for redraw events on a text window instance.
 *
 * \param instance		The text window instance to be redrawn.
 * \param *redraw		Pointer to the redraw data block.
 * \param *plotter		Pointer to a line plotter function.
 * \param *data			A data pointer to be passed to the plotter.
 */

void window_redraw(struct window_instance *instance, wimp_draw *redraw, osbool (*plotter)(int, struct window_redraw *, void *), void *data);


/**
 * Set the extent of a text block window.
 *
 * \param *instance		The instance to update.
 * \param lines			The number of lines to include.
 * */

void window_set_extent(struct window_instance *instance, int lines);
#endif
#endif
