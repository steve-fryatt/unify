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
 * \file: test_file.h
 *
 * Test File interface.
 */

#ifndef UNIFY_TEST_FILE
#define UNIFY_TEST_FILE

enum test_file_status {
	TEST_FILE_STATUS_NONE = 0,
	TEST_FILE_STATUS_SOURCE = 1,	/**< The test file has source.		*/
	TEST_FILE_STATUS_ABSOLUTE = 2	/**< The test file has an executable.	*/
};

struct test_file_block;

void test_file_delete_all(struct test_file_block **list);

void test_file_include_entry(struct test_file_block **list, enum test_file_status type, char* name, char *filename);

#endif
