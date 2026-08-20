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
 * \file: suite.h
 *
 * Test Suite interface.
 *
 * A Test Suite is a folder containing unit test source and executable files,
 * along with all of the associated data for them.
 *
 * It will contain a list of one or more File Sets, which contain the details
 * of the files within the suite at the times when an attempt was made to run
 * the tests.
 */

#ifndef UNIFY_SUITE
#define UNIFY_SUITE

/**
 * A test suite instance.
 */

struct suite_block;

/**
 * Create a new Test Suite instance and link it in to the collection of
 * active instances.
 *
 * \param *folder	Pointer to the name of the folder holding the
 *			test suite files.
 * \return		TRUE if successful; FALSE on error.
 */

osbool suite_create_instance(char *folder);

/**
 * Delete a Test Suite instance and delink it from the collection of
 * active instances.
 *
 * \param *instance	Pointer to the instance to be deleted.
 */

void suite_delete_instance(struct suite_block *instance);

/**
 * Delete all active Test Suite instances and free all resources
 * associated with them.
 */

void suite_delete_all(void);

/**
 * Store an item of text in the instance's text dump, returning the index of
 * the string.
 *
 * \param *instance	Poiinter to the Test Suite instance.
 * \param *text		Pointer to the text to be stored.
 * \return		The text dump offset, or TEXTDUMP_NULL.
 */

unsigned suite_store_text(struct suite_block *instance, char *text);

#endif
