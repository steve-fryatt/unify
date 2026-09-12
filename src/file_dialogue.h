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
 * \file: file_dialogue.h
 *
 * File Dialogue interface.
 */

#ifndef UNIFY_FILE_DIALOGUE
#define UNIFY_FILE_DIALOGUE

#include <stddef.h>
#include <stdint.h>

//#include "file_instance.h"

/**
 * The data required to populate a file dialogue.
 */

struct file_dialogue_data {
	char *name;
//	enum file_instance_status status;
	uint64_t run_timestamp;
	char *source_filename;
	uint64_t source_timestamp;
	char *executable_filename;
	uint64_t executable_timestamp;
};

/**
 * Initialise the file dialogue.
 */

void file_dialogue_initialise(void);

/**
 * Populate the details of the file information dialogue.
 *
 * \param *data		Pointer to a structure containing the details to be
 *			populated.
 */

void file_dialogue_populate(struct file_dialogue_data *data);

#endif
