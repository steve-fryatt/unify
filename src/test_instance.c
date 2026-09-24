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
 * \file: test_instance.c
 *
 * Test Instance implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/event.h>
#include <sflib/heap.h>

/* Application header files */

#include "test_instance.h"

#include "file_instance.h"
#include "suite.h"
#include "textdump.h"

/* Structure definitions. */

/* Global variables. */

/* Static function prototypes. */

/**
 * Return the details required for redrawing a display line of a test
 * instance
 *
 * \param *instance	Pointer to the test instance of interest.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool test_instance_get_line_details(struct test_instance_block *instance, struct file_instance_line_details *details)
{
	if (instance == NULL || details == NULL)
		return FALSE;

	details->name = instance->name;
	switch (instance->status) {
	case TEST_INSTANCE_STATUS_PASSED:
		details->status = FILE_INSTANCE_STATUS_PASS;
		break;
	case TEST_INSTANCE_STATUS_FAILED:
		details->status = FILE_INSTANCE_STATUS_FAIL;
		break;
	case TEST_INSTANCE_STATUS_SKIPPED:
		details->status = FILE_INSTANCE_STATUS_TEST_SKIPPED;
		break;
	case TEST_INSTANCE_STATUS_ERROR:
		details->status = FILE_INSTANCE_STATUS_TEST_ERROR;
		break;
	default:
		details->status = FILE_INSTANCE_STATUS_UNKNOWN;
		break;
	}
	return TRUE;
}

/**
 * Populate the contents of a new test instance.
 *
 * \param *instance	Pointer to the test instance to be populated.
 * \param name		Textdump offset to the name of the new instance.
 */

void test_instance_populate_new_test(struct test_instance_block *instance, unsigned name)
{
	if (instance == NULL)
		return;

	debug_printf("Populating new test... %s", name);

	instance->name = name;
	instance->location = TEST_INSTANCE_LOCATION_NONE;
	instance->status = TEST_INSTANCE_STATUS_UNKNOWN;
}

/**
 * Add a location for a test reference to a test instance, recording that the
 * test has been seen in this location.
 *
 * If the test has already been flagged as being seen in the location, failure
 * will be returned.
 *
 * \param *instance	Pointer to the test instance to be updated.
 * \param location	The location to be added to the test.
 * \param line		The line of the file at which the reference was found.
 * \return		TRUE if successful; FALSE if the test could not be
 *			updated.
 */

osbool test_instance_add_location(struct test_instance_block *instance, enum test_instance_location location, int line)
{
	if (instance == NULL)
		return FALSE;

	if ((instance->location & location) != TEST_INSTANCE_LOCATION_NONE)
		return FALSE;

	instance->location |= location;

	return TRUE;
}

/**
 * Update the status for a test instance.
 *
 * If the test status has already been changed from UNKNOWN, failure will be
 * returned.
 *
 * \param *instance	Pointer to the test instance to be updated.
 * \param status	The status to be set for the test.
 * \return		TRUE if successful; FALSE if the test could not be
 *			updated.
 */

osbool test_instance_update_status(struct test_instance_block *instance, enum test_instance_status status)
{
	if (instance == NULL)
		return FALSE;

	if (instance->status != TEST_INSTANCE_STATUS_UNKNOWN)
		return FALSE;

	instance->status = status;

	return TRUE;
}

/**
 * Validate a test at the end of execution, returning the status.
 *
 * \param *instance	Pointer to the test instance to be validated.
 * \return		The status of the instance after validation.
 */

enum test_instance_status test_instance_validate_test(struct test_instance_block *instance)
{
	if (instance == NULL)
		return TEST_INSTANCE_STATUS_UNKNOWN;

	/* The test should have been seen at source, call and result. */

	if (instance->location != TEST_INSTANCE_LOCATION_ALL)
		instance->status = TEST_INSTANCE_STATUS_ERROR;

	return instance->status;
}

/**
 * Compare a test with a name, to see if the two match.
 *
 * \param *instance	Pointer to the test instance of interest.
 * \param *suite	Pointer to the parent test suite.
 * \param *name		Pointer to the name to be tested.
 * \return		TRUE if the name and test match; otherwise FALSE.
 */

osbool test_instance_compare_test(struct test_instance_block *instance, struct suite_block *parent, char *name)
{
	if (instance == NULL)
		return FALSE;

	char *textdump_base = suite_get_textdump_base(parent);
	if (textdump_base == NULL || instance->name == TEXTDUMP_NULL)
		return FALSE;

	debug_printf("Comparing against %s", textdump_base + instance->name);

	if (strcmp(name, textdump_base + instance->name) == 0)
		return TRUE;

	return FALSE;
}
