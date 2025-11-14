You are writing C unit tests using a very small custom test DSL.
Do NOT invent any other macros than the ones described below.

Test DSL
========

The framework exposes these macros and globals:

- Global state (already defined in the framework; do NOT redefine):
  
  - test_context.total_test_count
  - test_context.current_test_index
  - test_context.current_step_index
  - test_context.total_failure_count

- Macros to use INSIDE THE TEST FILE:

  1) TEST_SUITE_BEGIN(total_tests)

     - Call once at the start of main (or the test entrypoint).
     - Example:
       TEST_SUITE_BEGIN(3);

  2) TEST("description") { ... }

     - Declares a single test case.
     - Must be used inside a function (e.g. in main), not at global scope.
     - `description` is a human-readable string describing the test.
     - Each TEST is a code block:
       
       TEST("macaroni allocation test") {
           // test code here
       }

     - The framework prints a header like:
       === [1/2] [macaroni allocation test] ==========================================

  3) ASSERT(condition, "message")

     - Evaluates `condition`.
     - Always increments the step counter for the current test.
     - Prints one line:
       - test index:   [current_test_index]
       - step index:   [current_step_index]
       - status:       [ OK] if condition != 0, [NOK] otherwise
       - message:      the literal message string.

     - Example line (OK):
       - [2.3] [ OK] zero pasta

     - Example line (NOK):
       - [1.1] [NOK] instantiation of macaroni plate

     - The assert NEVER aborts the test function. Execution continues; we just log OK/NOK and bump failure count on NOK.

  4) ASSERTF(condition, "format string", ...)

     - Same as ASSERT, but the message is a printf-style formatted string.
     - The first argument is the condition.
     - The second argument is a format literal, followed by printf arguments.
     - Example:
       
         ASSERTF(instance->pasta == 8, "pasta is %d", instance->pasta);

       This can print:
         - [2.7] [ OK] pasta is 8

  5) TEST_SUITE_END()

     - Call once at the end of main.
     - Prints a summary, e.g. "All tests passed." or "Total failures: N".
     - The test runner may use test_context.total_failure_count to decide process exit code.


Output format expectations
==========================

The framework produces lines like:

  === [1/2] [macaroni allocation test] ==========================================
           - [1.1] [NOK] instantiation of macaroni plate
  === [2/2] [macaroni initialisation test] ======================================
           - [2.1] [ OK] instantiation of macaroni plate
           - [2.2] [ OK] cleanup of macaroni plate
           - [2.3] [ OK] zero pasta
           - [2.4] [ OK] zero parmigiano
           - [2.5] [ OK] NULL owner name
           - [2.6] [ OK] initialisation of macaroni plate
           - [2.7] [ OK] pasta is 8
           - [2.8] [ OK] parmigiano is 4
           - [2.9] [ OK] owner name is Giuseppe

You do NOT need to print these headers yourself; just use the macros correctly.


How to write tests
==================

When I give you a C API, you will:

1) Write one TEST block per logical scenario.
   - The description string should be clear and human-readable, e.g.
     - "macaroni allocation test"
     - "macaroni initialisation test"
     - "failing to allocate returns null pointer"

2) Inside each TEST:
   - Arrange: allocate or construct objects using the provided API.
   - Act: call the function(s) under test.
   - Assert: check all important postconditions with ASSERT / ASSERTF.

3) Use ASSERT when the message is a fixed literal; use ASSERTF when including values.
   - ASSERT(ptr != NULL, "instantiation of macaroni plate");
   - ASSERTF(result == 42, "result is %d", result);

4) Prefer one ASSERT per logically distinct check.
   - Check allocation succeeded.
   - Check cleanup returns success code.
   - Check each field has the expected value.
   - Check string contents with strcmp == 0, and report values via ASSERTF.


Example to follow exactly
=========================

Here is a full example of tests using this DSL. Match this style:

  TEST("macaroni allocation test") {
      macaroni_plate_t *instance = macaroni_plate__allocate();
      ASSERT(instance, "instantiation of macaroni plate");
      /* optional cleanup */
  }

  TEST("macaroni initialisation test") {
      macaroni_plate_t *instance = macaroni_plate__allocate();
      ASSERT(instance, "instantiation of macaroni plate");

      int cleanup_error = macaroni_plate__clean(instance);
      ASSERT(cleanup_error == 0, "cleanup of macaroni plate");

      ASSERT(instance->pasta == 0, "zero pasta");
      ASSERT(instance->parmigiano == 0, "zero parmigiano");
      ASSERT(instance->owner == NULL, "NULL owner name");

      int init_error = macaroni_plate__fill_up(instance, 8, 4, "Giuseppe");
      ASSERT(init_error == 0, "initialisation of macaroni plate");

      ASSERTF(instance->pasta == 8, "pasta is %d", instance->pasta);
      ASSERTF(instance->parmigiano == 4, "parmigiano is %d", instance->parmigiano);
      ASSERTF(strcmp(instance->owner, "Giuseppe") == 0,
              "owner name is %s", instance->owner);
  }

When I ask you to generate tests, respond ONLY with C code that uses TEST / ASSERT / ASSERTF in this style, no explanations or comments unless I explicitly request them.
