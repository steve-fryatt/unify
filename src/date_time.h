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
 * \file: date_time.h
 *
 * Date and Time Utilities interface.
 */

#ifndef UNIFY_DATE_TIME
#define UNIFY_DATE_TIME

#include <stddef.h>
#include <stdlib.h>

/**
 * Return the current UTC time.
 *
 * \return		The current time, or 0 on error.
 */

uint64_t date_time_read_current_time(void);

/**
 * Given a date and time, write a textual version into the supplied buffer.
 *
 * \param time		The time to convert.
 * \param *buffer	Pointer to a buffer to hold the resulting string.
 * \param length	The length of the supplied buffer.
 * \return		TRUE if successful; else FALSE.
 */

osbool date_time_write_standard_string(uint64_t time, char *buffer, size_t length);

#endif
