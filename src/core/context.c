/*
 * @file context.c
 * @author a8kj7sea
 * @brief Implementation of the context initialization for Flatty.
 */

#include "core/context.h"
#include <stddef.h>

/*
 * @brief Initializes the Context struct with default values.
 * @param ctx Pointer to the Context struct to initialize.
 */
void context_init(Context *ctx)
{
  ctx->target_path = ".";
  ctx->output_file = "project_content.txt";
  ctx->ignore_file = NULL;
  ctx->config_file = NULL;
  ctx->cli_output_set = false;
  ctx->cli_ignore_set = false;
  ctx->cli_config_set = false;
}