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
 * \file: date_time.c
 *
 * Date and Time Utilities implementation.
 */

/* ANSI C header files */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>
#include <oslib/osword.h>
#include <oslib/territory.h>
#include <oslib/types.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>

/* Application header files */

#include "date_time.h"

/**
 * Return the current UTC time.
 *
 * \return		The current time, or 0 on error.
 */

uint64_t date_time_read_current_time(void)
{
	oswordreadclock_utc_block utc = { .op = oswordreadclock_OP_UTC };
	os_error *error = xoswordreadclock_utc(&utc);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return 0;
	}

	uint64_t time = ((uint64_t) utc.utc[0] << 0) |
			((uint64_t) utc.utc[1] << 8) |
			((uint64_t) utc.utc[2] << 16) |
			((uint64_t) utc.utc[3] << 24) |
			((uint64_t) utc.utc[4] << 32);

	debug_printf("Read time: %" PRId64, time);

	return time;
}

/**
 * Given a date and time, write a textual version into the supplied buffer.
 *
 * \param time		The time to convert.
 * \param *buffer	Pointer to a buffer to hold the resulting string.
 * \param length	The length of the supplied buffer.
 * \return		TRUE if successful; else FALSE.
 */

osbool date_time_write_standard_string(uint64_t time, char *buffer, size_t length)
{
	if (buffer == NULL || length == 0)
		return FALSE;

	debug_printf("Print time: %" PRId64, time);

	os_date_and_time os;

	os[0] = (time >> 0) & 0xffu;
	os[1] = (time >> 8) & 0xffu;
	os[2] = (time >> 16) & 0xffu;
	os[3] = (time >> 24) & 0xffu;
	os[4] = (time >> 32) & 0xffu;

	os_error *error = xterritory_convert_standard_date_and_time(territory_CURRENT,
			(const os_date_and_time *) &os, buffer, length, NULL);

	if (error != NULL) {
		buffer[0] = '\0';
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	return TRUE;
}
