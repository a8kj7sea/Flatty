/*
 * @file ignore_parser.c
 * @author a8kj7sea
 * @brief Implementation of the ignore rules parser using fnmatch.
 */

#include "ignore/ignore_parser.h"
#include "utils/string_utils.h"
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * @brief Initializes the IgnoreRules structure.
 * @param rules Pointer to the IgnoreRules to initialize.
 */
void ignore_rules_init(IgnoreRules *rules)
{
  rules->patterns = NULL;
  rules->count = 0;
  rules->capacity = 0;
}

/*
 * @brief Cleans up memory allocated for IgnoreRules.
 * @param rules Pointer to the IgnoreRules to clean up.
 */
void ignore_rules_cleanup(IgnoreRules *rules)
{
  if (!rules->patterns)
    return;
  for (size_t i = 0; i < rules->count; i++)
  {
    free(rules->patterns[i]);
  }
  free(rules->patterns);
  rules->patterns = NULL;
  rules->count = 0;
  rules->capacity = 0;
}

/*
 * @brief Adds a new ignore pattern to the IgnoreRules structure.
 * @param rules Pointer to the IgnoreRules.
 * @param pattern The ignore pattern string to add.
 * @return true on success, false on allocation failure.
 */
static bool add_pattern(IgnoreRules *rules, const char *pattern)
{
  if (rules->count == rules->capacity)
  {
    size_t new_cap = rules->capacity == 0 ? 8 : rules->capacity * 2;
    char **new_patterns = realloc(rules->patterns, new_cap * sizeof(char *));
    if (!new_patterns)
      return false;
    rules->patterns = new_patterns;
    rules->capacity = new_cap;
  }
  rules->patterns[rules->count] = strdup(pattern);
  if (!rules->patterns[rules->count])
    return false;
  rules->count++;
  return true;
}

/*
 * @brief Loads ignore patterns from a specified file.
 * @param rules Pointer to the IgnoreRules to populate.
 * @param file_path Path to the ignore file.
 * @return true on success, false on failure to open or read the file.
 */
bool ignore_rules_load_file(IgnoreRules *rules, const char *file_path)
{
  FILE *file = fopen(file_path, "r");
  if (!file)
    return false;

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline(&line, &len, file)) != -1)
  {
    char *trimmed = string_utils_trim(line);
    if (*trimmed == '\0' || *trimmed == '#')
      continue;
    if (*trimmed == '!')
    {
      fprintf(stderr, "Warning: Negation '!' is not supported. Skipping: %s\n",
              trimmed);
      continue;
    }
    if (!add_pattern(rules, trimmed))
    {
      fprintf(stderr, "Error: Failed to allocate memory for ignore pattern\n");
      free(line);
      fclose(file);
      return false;
    }
  }

  if (ferror(file))
  {
    free(line);
    fclose(file);
    return false;
  }

  free(line);
  fclose(file);
  return true;
}

/*
 * @brief Checks if a given relative path matches any of the ignore rules.
 * @param rules Pointer to the IgnoreRules to check against.
 * @param relative_path The path of the file/directory relative to the root.
 * @param is_directory True if the path is a directory, false otherwise.
 * @return true if the path matches an ignore rule, false otherwise.
 */
bool ignore_matches(const IgnoreRules *rules, const char *relative_path,
                    bool is_directory)
{
  for (size_t i = 0; i < rules->count; i++)
  {
    const char *pattern = rules->patterns[i];
    size_t pat_len = strlen(pattern);
    bool is_dir_rule = false;

    char match_pat[pat_len + 1];
    strcpy(match_pat, pattern);

    if (pat_len > 0 && match_pat[pat_len - 1] == '/')
    {
      is_dir_rule = true;
      match_pat[pat_len - 1] = '\0';
    }

    if (is_dir_rule && !is_directory)
      continue;

    bool has_slash = strchr(match_pat, '/') != NULL;

    if (has_slash)
    {
      if (fnmatch(match_pat, relative_path, FNM_PATHNAME) == 0)
        return true;
    }
    else
    {
      const char *basename = strrchr(relative_path, '/');
      basename = basename ? basename + 1 : relative_path;
      if (fnmatch(match_pat, basename, 0) == 0)
        return true;
    }
  }
  return false;
}