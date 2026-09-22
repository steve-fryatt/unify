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
#include "log.h"

/**
 * The types of project that we know about.
 */

enum project_type {
	PROJECT_TYPE_UNKNOWN,				/**< We don't know the project type.			*/
	PROJECT_TYPE_UNITY_GCCSDK_SFTOOLS		/**< C and Unity, built using the GCCSDK and SFTools.	*/
};

/**
 * The different types of test outcome that we can report back.
 */

enum project_outcome {
	PROJECT_OUTCOME_UNKNOWN,			/**< We don't know what the outcome was.		*/
	PROJECT_OUTCOME_PASS,				/**< The test passed.					*/
	PROJECT_OUTCOME_FAIL,				/**< The test failed.					*/
	PROJECT_OUTCOME_SKIP,				/**< The test was skipped.				*/
};

/**
 * Callbacks that source file parsers will need to use.
 */

struct project_source_callbacks {
	/**
	 * The file instance owning the source file.
	 */
	struct file_instance_block *owner;

	/**
	 * Callback to report that we've found a function definition.
	 *
	 * \param *owner	The file instance owning the source file, as
	 *			supplied above.
	 * \param *name		Pointer to the name of the test being defined.
	 * \param line		The line number where the definition was found
	 *			in the file, or -1 if unknown.
	 */
	void (*found_definition)(struct file_instance_block *owner, char *name, int line);

	/**
	 * Callback to report that we've found a function call.
	 *
	 * If the test framework doesn't have definitions and calls, then then
	 * this callback should be used at the same time as found_definition().
	 *
	 * \param *owner	The file instance owning the source file, as
	 *			supplied above.
	 * \param *name		Pointer to the name of the test being called.
	 * \param line		The line number where the call was found in the
	 *			file, or -1 if unknown.
	 */
	void (*found_call)(struct file_instance_block *owner, char *name, int line);
};

struct project_log_callbacks {
	/**
	 * The file instance owning the log.
	 */
	struct file_instance_block *owner;

	/**
	 * Callback to report that we've found the result of one of the tests.
	 *
	 * \param *owner	The file instance owning the log, as supplied
	 *			above.
	 * \param *name		Pointer to the name of the test being reported
	 *			on.
	 * \param line		The line number where the log reported the test
	 *			to have been within the source, or -1 if unknown.
	 * \param outcome	The outcome of the test.
	 */
	void (*found_test_result)(struct file_instance_block *owner, char *name, int line, enum project_outcome outcome);

	/**
	 * Callback to report that we've found the test summary.
	 *
	 * \param *owner	The file instance owning the log, as supplied
	 *			above.
	 * \param tests		The total number of tests to have been run, or
	 *			-1 if unknown.
	 * \param passed	The number of tests passed, or -1 if unknown.
	 * \param failed	The number of tests failed, or -1 if unknown.
	 * \param skipped	The number of tests skipped, or -1 if unknown.
	 */
	void (*found_summary)(struct file_instance_block *owner, int tests, int passed, int failed, int skipped);

	/**
	 * Callback to report that we've found the overall test outcome.
	 *
	 * \param *owner	The file instance owning the log, as supplied
	 *			above.
	 * \param outcome	The overall outcome of the test collection.
	 */

	void (*found_overall_result)(struct file_instance_block *owner, enum project_outcome outcome);
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

	/**
	 * The decoder for log output.
	 */
	osbool (*log_decoder)(struct log_instance *log, struct project_log_callbacks *callbacks);
};

/**
 * Given a project type, return a pointer to the project definition.
 *
 * \param type		The type of project to look for.
 * \return		Pointer to the project definition, or NULL on failure.
 */

struct project_details *project_get_definition(enum project_type type);

#endif
