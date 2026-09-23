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
 * \file: string_match.c
 *
 * String Matching implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/types.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/string.h>

/* Application header files */

#include "string_match.h"

/* Structure definitions. */


/* Global variables. */

static char *string_match_string = NULL;

static unsigned string_match_index = 0;

/* Static function prototypes. */


/**
 * Start a new string match operation.
 *
 * \param *string	Pointer to the string to match within.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_start(char *string)
{
	if (string == NULL)
		return FALSE;

	string_match_string = string;
	string_match_index = 0;

	return TRUE;
}

/**
 * Test whether the matching has reached the end of the line, allowing for some
 * trailing white space.
 *
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_test_end(void)
{
	if (string_match_string == NULL)
		return FALSE;

	if (!string_match_clear_whitespace())
		return FALSE;

	return (string_match_string[string_match_index] == '\0') ? TRUE : FALSE;
}

/**
 * Skip past a block of white space in the test line.
 *
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_clear_whitespace(void)
{
	if (string_match_string == NULL)
		return FALSE;

	while (string_match_string[string_match_index] != '\0' && isspace(string_match_string[string_match_index]))
		string_match_index++;

	return TRUE;
}

/**
 * Test for a given string at the current location in the test line, skipping
 * past if a match is found.
 *
 * \param *string	Pointer to the string to be tested for.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_test_string(char *string)
{
	if (string_match_string == NULL || string == NULL)
		return FALSE;

	while (string_match_string[string_match_index] != '\0' && *string != '\0' &&
			string_match_string[string_match_index] == *string) {

		string_match_index++;
		string++;
	}

	return (*string == '\0') ? TRUE : FALSE;
}

/**
 * Test for one of several option strings at the current location in the test
 * line, reporting the index into the array of options on return.
 *
 * If no match is found, this is treated as a failure.
 *
 * \param *options	Pointer to an array of pointers to possible option
 *			strings, terminated by a NULL.
 * \param *index	Pointer to a variable to take the index of the matched
 *			option, or -1 if there was no match.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_find_option(char *options[], int *index)
{
	if (string_match_string == NULL || options == NULL)
		return FALSE;

	int i = 0;
	char *match = NULL;

	while ((match = options[i]) != NULL) {
		char *line = string_match_string + string_match_index;

		while (*line != '\0' && *match != '\0' && *line == * match) {
			line++;
			match++;
		}

		if (*match == '\0') {
			if (index != NULL)
				*index = i;
			return TRUE;
		}

		i++;
	}

	if (index != NULL)
		*index = -1;

	return FALSE;
}

/**
 * Identify and return an integer number from the test line. Leading or trailing
 * white space will be removed as part of the operation.
 *
 * \param *index	Pointer to a variable to take the number read from the
 *			line
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_find_number(int *value)
{
	if (string_match_string == NULL)
		return FALSE;

	/* Evaluate the following text as a number. */

	errno = 0;
	char *end = NULL;
	long result = strtol(string_match_string + string_match_index, &end, 10);

	if (errno == ERANGE) {
		return FALSE;
	} else if (end == string_match_string + string_match_index) {
		return FALSE;
	}

	if (value != NULL)
		*value = (result >= INT_MIN && result <= INT_MAX) ? result : 0;

	/* Step past the number, and any following white space. */

	string_match_index = end - string_match_string;

	return string_match_clear_whitespace();
}

/**
 * Step forward in the test line until the given string is located. Optionally
 * read the intervening text into a buffer.
 *
 * \param *string	Pointer to the string to be located.
 * \param *buffer	Pointer to a buffer to take the intervening text, or
 *			NULL if the text should not be returned.
 * \param length	The length of the supplied buffer, or zero.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_skip_forward_to(char *string, char *buffer, size_t length)
{
	if (string_match_string == NULL || string == NULL)
		return FALSE;

	int copy_ptr = 0;

	while (string_match_string[string_match_index] != '\0') {
		if (string_match_string[string_match_index] == *string) {
			int a = string_match_index;
			char *b = string;

			while (string_match_string[a] != '\0' && *b != '\0' && string_match_string[a] == *b) {
				a++;
				b++;
			}

			if (*b == '\0') {
				string_match_index = a;

				if (buffer != NULL) {
					if (copy_ptr < length)
						buffer[copy_ptr] = '\0';
					else if (length > 0)
						buffer[0] = '\0';
				}
				return TRUE;
			}
		}

		if (buffer != NULL && copy_ptr < length)
			buffer[copy_ptr++] = string_match_string[string_match_index];

		string_match_index++;
	}

	if (buffer != NULL && length > 0)
		buffer[0] = '\0';

	return FALSE;
}
