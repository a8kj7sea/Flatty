/*
 * @file test_ignore.c
 * @author a8kj7sea
 * @brief Unit tests for the ignore rules parser and matcher.
 */

#include "ignore/ignore_parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * @brief Main function for the ignore rules test.
 * @return 0 on success.
 */
int main()
{
  IgnoreRules rules;
  ignore_rules_init(&rules);

  /* Simulate loading */
  rules.patterns = malloc(2 * sizeof(char *));
  rules.patterns[0] = strdup("*.log");
  rules.patterns[1] = strdup("build/");
  rules.count = 2;

  /* Basename match */
  assert(ignore_matches(&rules, "debug.log", false) == true);
  assert(ignore_matches(&rules, "src/debug.log", false) == true);

  /* Directory only match */
  assert(ignore_matches(&rules, "build", true) == true);
  assert(ignore_matches(&rules, "build", false) == false); /* Not a dir */
  assert(ignore_matches(&rules, "rebuild", true) ==
         false); /* Not exact basename */

  ignore_rules_cleanup(&rules);

  printf("Ignore rules tests passed.\n");
  return 0;
}