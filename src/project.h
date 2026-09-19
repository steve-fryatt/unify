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
 * \file: project.h
 *
 * Project-specific detail interface.
 */

#ifndef UNIFY_PROJECT
#define UNIFY_PROJECT

#include <oslib/types.h>
#include "file_instance.h"

/**
 * The types of project that we know about.
 */

enum project_type {
	PROJECT_TYPE_UNKNOWN,				/**< We don't know the project type.			*/
	PROJECT_TYPE_UNITY_GCCSDK_SFTOOLS		/**< C and Unity, built using the GCCSDK and SFTools.	*/
};

/**
 * Callbacks that source file parsers will need to use.
 */

struct project_source_callbacks {
	struct file_instance_block *owner;
	void (*found_definition)(struct file_instance_block *owner, char *name, int line);	/**< We've found a function definition.			*/
	void (*found_call)(struct file_instance_block *owner, char *name, int line);	/**< We've found a function call.			*/
};

/**
 * The details of a project type.
 */

struct project_details {
	/**
	 * The type of project.
	 */
	enum project_type type;

	/**
	 * The decoder for source files.
	 */
	osbool (*source_decoder)(FILE *f, struct project_source_callbacks *callbacks);
};

/**
 * Given a project type, return a pointer to the project definition.
 *
 * \param type		The type of project to look for.
 * \return		Pointer to the project definition, or NULL on failure.
 */

struct project_details *project_get_definition(enum project_type type);

#endif
