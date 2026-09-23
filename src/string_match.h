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
 * \file: string_match.h
 *
 * String Matching interface.
 */

#ifndef UNIFY_STRING_MATCH
#define UNIFY_STRING_MATCH

/**
 * Start a new string match operation.
 *
 * \param *string	Pointer to the string to match within.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_start(char *string);

/**
 * Test whether the matching has reached the end of the line, allowing for some
 * trailing white space.
 *
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_test_end(void);

/**
 * Skip past a block of white space in the test line.
 *
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_clear_whitespace(void);

/**
 * Test for a given string at the current location in the test line, skipping
 * past if a match is found.
 *
 * \param *string	Pointer to the string to be tested for.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_test_string(char *string);

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

osbool string_match_find_option(char *options[], int *index);

/**
 * Identify and return an integer number from the test line. Leading or trailing
 * white space will be removed as part of the operation.
 *
 * \param *index	Pointer to a variable to take the number read from the
 *			line
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_find_number(int *value);

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

osbool string_match_skip_forward_to(char *string, char *buffer, size_t length);

#endif
