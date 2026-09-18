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
#include <sflib/string.h>

/* Application header files */

#include "runner.h"

#include "file_instance.h"

/**
 * The number of tasks that we will launch in parallel.
 */
#define RUNNER_TASKS 10

/* Structure definitions. */

struct runner_job {
	unsigned id;
	wimp_t child_handle;
	wimp_t task_handle;
	char *command;
	struct file_instance_block *owner;
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

static void runner_delete_task(struct runner_job *job);
static osbool runner_start_task(struct runner_job *job, int slot);
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

/**
 * TODO
 */

osbool runner_add_task(char *command, struct file_instance_block *owner)
{
	debug_printf("\\RExecuting %s", command);

	struct runner_job *new = heap_alloc(sizeof(struct runner_job));
	if (new == NULL)
		return FALSE;

	new->id = next_id++;
	new->task_handle = NULL;
	new->command = heap_strdup(command);
	new->owner = owner;
	new->next = NULL;

	if (new->command == NULL) {
		runner_delete_task(new);
		return FALSE;
	}

	/* See if we have a free slot to run the task now. */

	int slot = -1;

	for (int i = 0; i < RUNNER_TASKS; i++) {
		if (runner_active_jobs[i] == NULL) {
			slot = i;
			break;
		}
	}

	if (slot >= 0)
		return runner_start_task(new, slot);

	/* Link the task into the queue tail. */

	if (runner_queue_tail != NULL)
		runner_queue_tail->next = new;

	runner_queue_tail = new;

	/* If the queue is empty, make the task the head, too. */

	if (runner_queue_head == NULL)
		runner_queue_head = new;

	return TRUE;
}

/**
 * TODO
 */

static void runner_delete_task(struct runner_job *job)
{
	if (job == NULL)
		return;

	if (job->command != NULL)
		heap_free(job->command);

	heap_free(job);
}

/**
 * TODO
 */

static osbool runner_start_task(struct runner_job *job, int slot)
{
	char command[1024];

	string_printf(command, 2014,
			"TaskWindow \"%s\" -wimpslot 1024K -name \"Unit Test\" -quit -task &%08x -txt &%08x",
			job->command,
			runner_task_handle,
			job->id
	);

	os_error *error = xwimp_start_task(command, &(job->child_handle));

	debug_printf("Launched %s", command);
	debug_printf("Result = 0x%x, Child = 0x%x", error, job->child_handle);

	if (error != NULL)
		return FALSE;

	runner_active_jobs[slot] = job;

	return TRUE;
}

/**
 * TODO
 */

static osbool runner_task_window_ego(wimp_message *message)
{
	taskwindow_full_message_ego *ego = (taskwindow_full_message_ego *) message;

	debug_printf("Message_TaskWindowEgo, txt=0x%x", ego->txt);

	for (int slot = 0; slot < RUNNER_TASKS; slot++) {
		struct runner_job *job = runner_active_jobs[slot];
		if (job == NULL || job->id != ego->txt)
			continue;

		job->task_handle = ego->sender;
		debug_printf("Found id %d, matched child handle 0x%x and task handle 0x%x",
				job->id, job->child_handle, job->task_handle);
		break;
	}

	return TRUE;
}

/**
 * TODO
 */

static osbool runner_task_window_morio(wimp_message *message)
{
	debug_printf("Message_TaskWindowMorio");

	for (int slot = 0; slot < RUNNER_TASKS; slot++) {
		struct runner_job *job = runner_active_jobs[slot];
		if (job == NULL || job->task_handle != message->sender)
			continue;

		debug_printf("Id %d completed", job->id);

		/* End the current task. */

		file_instance_finish_execution(job->owner);

		runner_active_jobs[slot] = NULL;
		runner_delete_task(job);

		/* See if there's a task to launch in its place. */

		if (runner_queue_head != NULL) {
			struct runner_job *new = runner_queue_head;
			runner_queue_head = new->next;

			if (runner_queue_tail == new)
				runner_queue_tail = NULL;

			runner_start_task(new, slot);
		}

		break;
	}

	return TRUE;
}

/**
 * TODO
 */

static osbool runner_task_window_output(wimp_message *message)
{
	taskwindow_full_message_data *data = (taskwindow_full_message_data *) message;

	for (int slot = 0; slot < RUNNER_TASKS; slot++) {
		struct runner_job *job = runner_active_jobs[slot];
		if (job == NULL || job->task_handle != message->sender)
			continue;

		debug_printf("Id %d has data", job->id);

		file_instance_take_log_content(job->owner, data->data, data->data_size);
		break;
	}

	return TRUE;
}
