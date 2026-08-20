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
 * \file: file_instance.h
 *
 * Test File interface.
 */

#ifndef UNIFY_FILE_INSTANCE
#define UNIFY_FILE_INSTANCE

enum file_instance_status {
	FILE_INSTANCE_STATUS_NONE = 0,
	FILE_INSTANCE_STATUS_SOURCE = 1,	/**< The test file has source.		*/
	FILE_INSTANCE_STATUS_ABSOLUTE = 2	/**< The test file has an executable.	*/
};

struct file_instance_block;

/**
 * Initialise the Test File code.
 */

void file_instance_initialise(void);

void file_instance_delete_all(struct file_instance_block **list);

void file_instance_include_entry(struct file_instance_block **list, enum file_instance_status type, char* name, char *filename);

osbool file_instance_execute(struct file_instance_block *instance);

#endif
