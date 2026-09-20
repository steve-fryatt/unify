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
 * \file: project.c
 *
 * Project-specific detail implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/types.h>

/* SF-Lib header files. */

/* Application header files */

#include "project.h"

#include "file_instance.h"

/**
 * The maximum length of a function name.
 */

#define PROJECT_MAX_FUNCTION_LEN 1024

/* Structure definitions. */


/* Global variables. */

/* Static function prototypes. */

static osbool project_unity_scan_source(FILE *fh, struct project_source_callbacks *callbacks);
static osbool project_unity_scan_block(FILE *fh, int level, struct project_source_callbacks *callbacks);
static osbool project_unity_found_definition(FILE *fh, struct project_source_callbacks *callbacks);
static osbool project_unity_found_call(FILE *fh, struct project_source_callbacks *callbacks);

/**
 * The project defintions.
 */

static struct project_details project_definitions[] = {
	{
		.type = PROJECT_TYPE_UNITY_GCCSDK_SFTOOLS,
		.source_decoder = project_unity_scan_source
	},
	{
		.type = PROJECT_TYPE_UNKNOWN
	}
};

/**
 * Given a project type, return a pointer to the project definition.
 *
 * \param type		The type of project to look for.
 * \return		Pointer to the project definition, or NULL on failure.
 */

struct project_details *project_get_definition(enum project_type type)
{
	for (int i = 0; project_definitions[i].type != PROJECT_TYPE_UNKNOWN; i++) {
		if (project_definitions[i].type == type)
			return &(project_definitions[i]);
	}

	return NULL;
}

/*******************************************************************************
 * The Unity Project
 *
 * Decode C projects writtin using the Unity framework.
 */

/**
 * Scan a source file from a Unity project.
 *
 * \param *fh		The handle of the source file to be scanned.
 * \param *callbacks	Pointer to details of the callbacks to be used.
 * \return		TRUE if successful; FALSE on failure.
 */

static osbool project_unity_scan_source(FILE *fh, struct project_source_callbacks *callbacks)
{
	if (fh == NULL || callbacks == NULL)
		return FALSE;

	return project_unity_scan_block(fh, 0, callbacks);
}

/**
 * Scan a bracketed block (ie. text within { and }) within a file.
 *
 * \param *fh		The handle of the source file to be scanned.
 * \param level		The nesting level of the block, starting with zero as
 *			the top level of the file.
 * \param *callbacks	Pointer to details of the callbacks to be used.
 * \return		TRUE if successful; FALSE on failure.
 */

static osbool project_unity_scan_block(FILE *fh, int level, struct project_source_callbacks *callbacks)
{
	if (fh == NULL)
		return FALSE;

	int c;

	/* Step past any leading white space. */

	while ((c = fgetc(fh)) != EOF && isspace(c));
	if (c == EOF)
		return TRUE;

	fseek(fh, -1, SEEK_CUR);

	/* Scan for text that we're interested in. */

	char *definition = "void ";
	char *call = "RUN_TEST(";

	char *test_definition = definition, *test_call = call;

	while ((c = fgetc(fh)) != EOF && c != '}') {
		if (level == 1 && test_definition == definition && *test_call == c) {
			/* We're matching a call line. */
			test_call++;
			if (*test_call == '\0')
				project_unity_found_call(fh, callbacks);
		} else if (level == 0 && test_call == call && *test_definition == c) {
			/* We're matching a function definition line. */
			test_definition++;
			if (*test_definition == '\0')
				project_unity_found_definition(fh, callbacks);
		} else if (c == '{') {
			/* We've moved into a new block. */
			project_unity_scan_block(fh, level + 1, callbacks);
			test_definition = definition;
			test_call = call;
		} else if (c == ';') {
			/* The end of the current statement. */
			test_definition = definition;
			test_call = call;

			/* Skip past leading whitespace. */
			while ((c = fgetc(fh)) != EOF && isspace(c));
			if (c == EOF)
				break;

			fseek(fh, -1, SEEK_CUR);
		} else {
			/* All matches failed, so reset the searches. */
			test_definition = definition;
			test_call = call;
		}
	}

	return TRUE;
}

/**
 * We think that we have found a function definition, so extract the function
 * name and pass it back to the caller.
 *
 * \param *fh		The handle of the source file to be scanned.
 * \param *callbacks	Pointer to details of the callbacks to be used.
 * \return		TRUE if successful; FALSE on failure.
 */

static osbool project_unity_found_definition(FILE *fh, struct project_source_callbacks *callbacks)
{
	char buffer[PROJECT_MAX_FUNCTION_LEN], *b = buffer;

	int c = '\0';

	while ((c = fgetc(fh)) && c != '(' && c != ';')
		if (b < buffer + (sizeof(buffer) - 1))
			*b++ = c;

	*b = '\0';

	if (c == ';')
		fseek(fh, -1, SEEK_CUR);
	else if (c == '(' && strcmp(buffer, "main") && strcmp(buffer, "setUp") && strcmp(buffer, "tearDown"))
		callbacks->found_definition(callbacks->owner, buffer, -1);

	return (c == '(') ? TRUE : FALSE;
}

/**
 * We think that we have found a function call, so extract the function
 * name and pass it back to the caller.
 *
 * \param *fh		The handle of the source file to be scanned.
 * \param *callbacks	Pointer to details of the callbacks to be used.
 * \return		TRUE if successful; FALSE on failure.
 */

static osbool project_unity_found_call(FILE *fh, struct project_source_callbacks *callbacks)
{
	char buffer[PROJECT_MAX_FUNCTION_LEN], *b = buffer;

	int c = '\0';

	while ((c = fgetc(fh)) && c != ')' && c != ';')
		if (b < buffer + (sizeof(buffer) -1))
			*b++ = c;

	*b = '\0';

	if (c == ';')
		fseek(fh, -1, SEEK_CUR);
	else if (c == ')')
		callbacks->found_call(callbacks->owner, buffer, -1);

	return (c == ')') ? TRUE : FALSE;
}
