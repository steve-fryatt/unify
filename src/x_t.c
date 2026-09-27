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
 * \file: x.c
 *
 * X implementation.
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

#include "block.h"

#include "flexutils.h"
#include "log.h"

/* Structure definitions. */


/* Global variables. */

static wimp_t protocol_task_handle = NULL;

/* Static function prototypes. */

static osbool protocol_message_data_load(wimp_message *message);
static void protocol_log_message(struct log_set *log, wimp_message *message, int length);

/**
 * Initialise the protocol implementation.
 *
 * \param task_handle		The handle of the task.
 */

void protocol_initialise(wimp_t task_handle)
{
	protocol_task_handle = task_handle;

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, protocol_message_data_load);
}
