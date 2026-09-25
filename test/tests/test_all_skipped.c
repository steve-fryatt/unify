/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Unify:
 *
 *   http://www.stevefryatt.org.uk/software/
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
 * \file: test_all_skipped.c
 *
 * Unit tests which are all skipped.
 */

/* ANSI C Header files. */

/* Acorn C Header files. */

/* SFLib Header files. */

/* Unity Header files. */

#include "unity.h"

/* Unify Application header file. */

/**
 * Unit Test setup.
 */

void setUp(void)
{ }

/**
 * Unit Test teardown.
 */

void tearDown(void)
{ }

/**
 * The Unit Tests.
 */

void test_skip1(void)
{
	TEST_IGNORE();
}

void test_skip2(void)
{
	TEST_IGNORE_MESSAGE("This test is skipped.");
}

void test_skip3(void)
{
	TEST_IGNORE();
}

void test_skip4(void)
{
	TEST_IGNORE();
}

/**
 * The main test runner.
 */

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_skip1);
	RUN_TEST(test_skip2);
	RUN_TEST(test_skip3);
	RUN_TEST(test_skip4);
	return UNITY_END();
}
