#ifndef FLATTY_CONTEXT_H
#define FLATTY_CONTEXT_H

#include <stdbool.h>

/*
 * @brief Context structure holding command-line arguments and parsed states.
 */
typedef struct
{
  const char *target_path;
  const char *output_file;
  const char *ignore_file;
  const char *config_file;
  bool cli_output_set;
  bool cli_ignore_set;
  bool cli_config_set;
} Context;

/*
 * @brief Initializes the Context struct with default values.
 * @param ctx Pointer to the Context struct to initialize.
 */
void context_init(Context *ctx);

#endif