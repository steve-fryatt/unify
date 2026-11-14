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
 * \file: test_info.c
 *
 * Test Information window implementation.
 */

/* ANSI C header files */

#include <stddef.h>

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/heap.h>

/* Application header files */

#include "test_info.h"

/* Definitions. */

/**
 * A test info window instance.
 */

struct test_info_block {
	int none;
};

/* Global variables. */

/* Static function prototypes. */

/**
 * Construct a new test information instance.
 *
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct test_info_block *test_info_create_instance(void)
{
	struct test_info_block *new = heap_alloc(sizeof(struct test_info_block));
	if (new == NULL)
		return NULL;

	new->none = 0;

	debug_printf("\\PCreated test info instance 0x%x", new);

	return new;
}

/**
 * Delete a test information instance, closing the window and freeing up any
 * associated memory.
 *
 * \param *instance	Pointer to the instance to be freed.
 */

void test_info_delete_instance(struct test_info_block *instance)
{
	if (instance == NULL)
		return;

	debug_printf("\\pDeleting test info instance 0x%x", instance);

	heap_free(instance);
}

/**
 * Bring an instance's window to the front of the stack.
 *
 * \param *instance	Pointer to the instance to be accessed.
 */

void test_info_bring_to_front(struct test_info_block *instance)
{
	if (instance == NULL)
		return;

	debug_printf("\\vBringing test info instance 0x%x to the front", instance);
}

/* Call file_instance_close_test() to handle window closure. If we pass parent
 * instance and entry to the window object, it could handle this for us.
 */
