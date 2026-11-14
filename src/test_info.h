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
 * \file: test_info.h
 *
 * Test Information window interface.
 */

#ifndef UNIFY_TEST_INFO
#define UNIFY_TEST_INFO

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

/* Application header files */

/* Definitions. */

/**
 * A test info window instance.
 */

struct test_info_block;

/* Global variables. */

/* Function prototypes. */

/**
 * Construct a new test inforomation instance.
 *
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct test_info_block *test_info_create_instance(void);

/**
 * Delete a test information instance, closing the window and freeing up any
 * associated memory.
 *
 * \param *instance	Pointer to the instance to be freed.
 */

void test_info_delete_instance(struct test_info_block *instance);

/**
 * Bring an instance's window to the front of the stack.
 *
 * \param *instance	Pointer to the instance to be accessed.
 */

void test_info_bring_to_front(struct test_info_block *instance);

#endif