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
 * \file: test_outcomes.c
 *
 * Unit tests which generate some different outcomes.
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

/* This test will always fail. */

void test_fail(void)
{
	TEST_FAIL_MESSAGE("This test fails.");
}

/* This test will always pass. */

void test_pass(void)
{
	TEST_PASS_MESSAGE("This test passes.");
}

/* This test is skipped. */

void test_skip(void)
{
	TEST_IGNORE_MESSAGE("This test is ignored.");
}

/* This test is never run. */

void test_ignored(void)
{
	TEST_PASS_MESSAGE("This test would pass, if it was called");
}

/**
 * The main test runner.
 */

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_pass);
	RUN_TEST(test_fail);
	RUN_TEST(test_skip);
	return UNITY_END();
}
