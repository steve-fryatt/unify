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

#include <stdio.h>

/**
 * A non-log line
 */

#define LOG_NO_LINE ((unsigned) 0xffffffffu)

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
 * \param *title		Pointer to the title to use for the log.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct log_instance *log_create_instance(char *title);

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

/**
 * Add a block of text to the log instance. Text may contain control characters,
 * and does not need to be terminated: the specified number of bytes will be
 * copied.
 *
 * \param *instance		Pointer to the instance to take the text.
 * \param *content		Pointer to the content to be added.
 * \param length		The number of bytes in the content.
 */

void log_add_text(struct log_instance *instance, char *content, size_t length);

/**
 * Complete the addition of text to the log instance. This will cause the
 * content to be formatted and prepared for display.
 *
 * \param *instance		Pointer to the instance to be completed.
 */

void log_finish_text(struct log_instance *instance);

/**
 * Write a log to a file handle.
 *
 * \param *instance		Pointer to the log instance to be written.
 * \param *file			Pointer to the file handle to write to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool log_write_to_file(struct log_instance *instance, FILE *file);

/**
 * Read a line from a log file. This should be called repeatedly until all
 * of the available lines have been read.
 *
 * Lines can not be read until log_finish_text() has been called.
 *
 * \param *instance		Pointer to the log instance to be read.
 * \return			A line index if a new line is available, or
 *				LOG_NO_LINE otherwise.
 */
unsigned log_read_line(struct log_instance *instance);

/**
 * Obtain a pointer to a line of log text, given a log line returned by
 * log_read_line().
 *
 * Note that these pointers are into a flex heap, so they should not be
 * retained and used across any operation which might shift the heap.
 *
 * \param *instance		Pointer to the log instance to be read.
 * \param line			The line index of interest.
 * \return			Pointer to the line, or NULL.
 */

char *log_get_line_pointer(struct log_instance *instance, unsigned line);

#endif
