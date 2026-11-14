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
 * \file: file_info.h
 *
 * File Information window interface.
 */

#ifndef UNIFY_FILE_INFO
#define UNIFY_FILE_INFO

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

/* Application header files */

/* Definitions. */

/**
 * A file info window instance.
 */

struct file_info_block;

/* Global variables. */

/* Function prototypes. */

/**
 * Construct a new file information instance.
 *
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct file_info_block *file_info_create_instance(void);

/**
 * Delete a file information instance, closing the window and freeing up any
 * associated memory.
 *
 * \param *instance	Pointer to the instance to be freed.
 */

void file_info_delete_instance(struct file_info_block *instance);

/**
 * Bring an instance's window to the front of the stack.
 *
 * \param *instance	Pointer to the instance to be accessed.
 */

void file_info_bring_to_front(struct file_info_block *instance);

#endif