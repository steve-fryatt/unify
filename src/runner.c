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
 * \file: runner.c
 *
 * Test Runner implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>
#include <oslib/taskwindow.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/event.h>
#include <sflib/heap.h>

/* Application header files */

#include "runner.h"

/**
 * The number of tasks that we will launch in parallel.
 */
#define RUNNER_TASKS 10

/* Structure definitions. */

struct runner_job {
	unsigned id;
	wimp_t task_handle;
	char *command;
	struct runner_job *next;
};

/* Global variables. */

/**
 * The handle of our task.
 */
static wimp_t runner_task_handle = NULL;

/**
 * Sequential IDs to allocate to jobs.
 */
static unsigned next_id = 0x0u;

/**
 * The head of the runner queue, where jobs can be removed.
 */
static struct runner_job *runner_queue_head = NULL;

/**
 * The tail of the runner queue, where jobs can be added.
 */
static struct runner_job *runner_queue_tail = NULL;

/**
 * The collection of active jobs.
 */
static struct runner_job *runner_active_jobs[RUNNER_TASKS] = { NULL };

/* Static function prototypes. */

static osbool runner_task_window_ego(wimp_message *message);
static osbool runner_task_window_morio(wimp_message *message);
static osbool runner_task_window_output(wimp_message *message);

/**
 * Initialise the test runner implementation.
 *
 * \param task_handle		The handle of the task.
 */

void runner_initialise(wimp_t task_handle)
{
	runner_task_handle = task_handle;

	event_add_message_handler(message_TASK_WINDOW_EGO, EVENT_MESSAGE_INCOMING, runner_task_window_ego);
	event_add_message_handler(message_TASK_WINDOW_MORIO, EVENT_MESSAGE_INCOMING, runner_task_window_morio);
	event_add_message_handler(message_TASK_WINDOW_OUTPUT, EVENT_MESSAGE_INCOMING, runner_task_window_output);
}

osbool runner_add_task(char *command)
{
	struct runner_job *new = heap_alloc(sizeof(struct runner_job));
	if (new == NULL)
		return FALSE;

	new->id = next_id++;
	new->task_handle = NULL;
	new->command = heap_strdup(command);
	new->next = NULL;

	/* Link the task into the queue tail. */

	if (runner_queue_tail != NULL)
		runner_queue_tail->next = new;

	runner_queue_tail = new;

	/* If the queue is empty, make the task the head, too. */

	if (runner_queue_head == NULL)
		runner_queue_head = new;

	// TODO - Try to add the task into the active tasks. */
}

/**
 * TODO
 */

static osbool runner_task_window_ego(wimp_message *message)
{
	taskwindow_full_message_ego *ego = (taskwindow_full_message_ego *) message;

	debug_printf("Message_TaskWindowEgo, txt=0x%x", ego->txt);
	return TRUE;
}

/**
 * TODO
 */

static osbool runner_task_window_morio(wimp_message *message)
{
	debug_printf("Message_TaskWindowMorio");
	return TRUE;
}

/**
 * TODO
 */

static osbool runner_task_window_output(wimp_message *message)
{
	taskwindow_full_message_data *data = (taskwindow_full_message_data *) message;

	char buffer[256];

	string_copy(buffer, data->data, data->data_size);

	debug_printf("Message_TaskWindowOutput (%d): %s", data->data_size, buffer);
	return TRUE;
}
