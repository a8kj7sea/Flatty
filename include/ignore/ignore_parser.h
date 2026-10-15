#ifndef FLATTY_IGNORE_PARSER_H
#define FLATTY_IGNORE_PARSER_H

#include <stdbool.h>
#include <stddef.h>

/*
 * @brief Structure holding ignore patterns loaded from ignore files.
 */
typedef struct
{
  char **patterns;
  size_t count;
  size_t capacity;
} IgnoreRules;

/*
 * @brief Initializes the IgnoreRules structure.
 * @param rules Pointer to the IgnoreRules to initialize.
 */
void ignore_rules_init(IgnoreRules *rules);

/*
 * @brief Cleans up memory allocated for IgnoreRules.
 * @param rules Pointer to the IgnoreRules to clean up.
 */
void ignore_rules_cleanup(IgnoreRules *rules);

/*
 * @brief Loads ignore patterns from a specified file.
 * @param rules Pointer to the IgnoreRules to populate.
 * @param file_path Path to the ignore file.
 * @return true on success, false on failure to open or read the file.
 */
bool ignore_rules_load_file(IgnoreRules *rules, const char *file_path);

/*
 * @brief Checks if a given relative path matches any of the ignore rules.
 * @param rules Pointer to the IgnoreRules to check against.
 * @param relative_path The path of the file/directory relative to the root.
 * @param is_directory True if the path is a directory, false otherwise.
 * @return true if the path matches an ignore rule, false otherwise.
 */
bool ignore_matches(const IgnoreRules *rules, const char *relative_path,
                    bool is_directory);

#endif