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
 * \file: log_window.h
 *
 * Log Window interface.
 */

#ifndef UNIFY_LOG_WINDOW
#define UNIFY_LOG_WINDOW

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include <oslib/osspriteop.h>

/* SF-Lib header files. */

/* Application header files */

/* Definitions. */

/**
 * A Log Window instance.
 */

struct log_window_block;

/**
 * A Log Window client definition.
 */

struct log_window_definition {
	/**
	 * Callack when the window closes.
	 */
	void (*callback_close)(void *data);
};

/* Global variables. */

/* Function prototypes. */



/**
 * Initialise the log window implementation
 *
 * \param *sprites	Pointer to the user sprite area.
 */

void log_window_initialise(osspriteop_area *sprites);

/**
 * Create a new Log Window instance.
 *
 * \param *definition	Pointer to the client window definition.
 * \param *title	Pointer to the title to use for the window.
 * \param *data		Pointer to the client data.
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct log_window_block *log_window_create_instance(struct log_window_definition *definition, char *title, void *data);

/**
 * Delete a Log Window instance.
 *
 * \param *instance	Pointer to the instance to be deleted.
 */

void log_window_delete_instance(struct log_window_block *instance);

#endif