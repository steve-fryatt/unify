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
 * \file: runner.h
 *
 * Test Runner interface.
 */

#ifndef UNIFY_TEST_RUNNER
#define UNIFY_TEST_RUNNER

#include <oslib/wimp.h>

#include "file_instance.h"

/**
 * Initialise the test runner implementation.
 *
 * \param task_handle		The handle of the task.
 */

void runner_initialise(wimp_t task_handle);

/**
 * Add a task to the runner queue, to be executed when a slot becomes available.
 *
 * \param *command	Pointer to the command string which will launch the
 *			task.
 * \param *owner	Pointer to the file instance which will own the task.
 * \return		TRUE if the task was added to the queue; otherwise
 *			FALSE.
 */

osbool runner_add_task(char *command, struct file_instance_block *owner);

#endif
