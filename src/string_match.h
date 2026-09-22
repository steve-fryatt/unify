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
 * Test for a given string at the current location in the lest line, skipping
 * past if a match is found.
 *
 * \param *string	Pointer to the string to be tested for.
 * \return		TRUE on success; FALSE on failure.
 */

osbool string_match_test_string(char *string);

/**
 * TODO
 */

osbool string_match_find_option(char *options[], int *index);

/**
 * TODO
 */

osbool string_match_find_number(int *value);

/**
 * TODO
 */

osbool string_match_skip_forward_to(char *string, char *buffer, size_t length);

#endif
