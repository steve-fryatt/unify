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
 * \file: test_log.h
 *
 * Test log interface.
 */

#ifndef UNIFY_TEST_LOG
#define UNIFY_TEST_LOG

#include <stdio.h>
#include <oslib/types.h>
#include "log.h"

/**
 * A log instance.
 */

struct test_log_instance;

/**
 * Initialise the log implementation.
 */

void test_log_initialise(void);

/**
 * Create a new log instance.
 *
 * \param *title		Pointer to the title to use for the log.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct test_log_instance *test_log_create_instance(char *title);

/**
 * Destroy a log instance.
 *
 * \param *instance		The instance to be deleted.
 */

void test_log_delete_instance(struct test_log_instance *instance);

/**
 * Open (or re-open) a window for a log instance.
 *
 * \param *instance		The instance for which to open the window.
 */

void test_log_open_window(struct test_log_instance *instance);

/**
 * Add a block of text to the log instance. Text may contain control characters,
 * and does not need to be terminated: the specified number of bytes will be
 * copied.
 *
 * \param *instance		Pointer to the instance to take the text.
 * \param *content		Pointer to the content to be added.
 * \param length		The number of bytes in the content.
 */

void test_log_add_text(struct test_log_instance *instance, char *content, size_t length);

/**
 * Write a log to a file handle.
 *
 * \param *instance		Pointer to the log instance to be written.
 * \param *file			Pointer to the file handle to write to.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool test_log_write_to_file(struct test_log_instance *instance, FILE *file);

#endif
