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
 * Log storage and display interface.
 */

#ifndef UNIFY_LOG
#define UNIFY_LOG

/**
 * A log instance.
 */

struct log_instance;

/**
 * Initialise the log implementation.
 */

void log_initialise(void);

/**
 * Create a new log instance.
 *
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct log_instance *log_create_instance(void);

/**
 * Destroy a log instance.
 *
 * \param *instance		The instance to be deleted.
 */

void log_delete_instance(struct log_instance *instance);

/**
 * Open (or re-open) a window for a log instance.
 *
 * \param *instance		The instance for which to open the window.
 */

void log_open_window(struct log_instance *instance);

#endif
