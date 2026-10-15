/*
 * @file main.c
 * @author a8kj7sea
 * @brief Main entry point for the Flatty CLI tool.
 *        Parses arguments, loads config/ignore rules, and orchestrates the packing process.
 */

#define _GNU_SOURCE
#include "core/config_loader.h"
#include "core/context.h"
#include "core/walker.h"
#include "core/writer.h"
#include "flatty_config.h"
#include "ignore/ignore_parser.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * @brief Prints the help message for the Flatty CLI tool.
 */
void print_help(void)
{
  printf("Flatty - Flatten your project into a single context file\n\n");
  printf("Usage: flatty [PATH] [OPTIONS]\n\n");
  printf("Options:\n");
  printf("  -o, --output <file>     Specify output file\n");
  printf("  -i, --ignore <file>    Specify custom ignore file\n");
  printf("  -c, --config <file>    Specify custom config file\n");
  printf("  -h, --help             Show this help message\n");
}

/*
 * @brief Main function.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */
int main(int argc, char *argv[])
{
  Context ctx;
  context_init(&ctx);

  static struct option long_options[] = {{"output", required_argument, 0, 'o'},
                                         {"ignore", required_argument, 0, 'i'},
                                         {"config", required_argument, 0, 'c'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "o:i:c:h", long_options, NULL)) != -1)
  {
    switch (opt)
    {
    case 'o':
      ctx.output_file = optarg;
      ctx.cli_output_set = true;
      break;
    case 'i':
      ctx.ignore_file = optarg;
      ctx.cli_ignore_set = true;
      break;
    case 'c':
      ctx.config_file = optarg;
      ctx.cli_config_set = true;
      break;
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    default:
      fprintf(stderr, "Unknown option. Use -h for help.\n");
      return EXIT_FAILURE;
    }
  }

  if (optind < argc)
  {
    ctx.target_path = argv[optind];
    if (optind + 1 < argc)
    {
      fprintf(stderr, "Error: Multiple target paths specified.\n");
      return EXIT_FAILURE;
    }
  }

  struct stat path_stat;
  if (stat(ctx.target_path, &path_stat) != 0 || !S_ISDIR(path_stat.st_mode))
  {
    fprintf(stderr, "Error: Target path '%s' is not a valid directory.\n",
            ctx.target_path);
    return EXIT_FAILURE;
  }

  Config cfg;
  config_init(&cfg);
  const char *defaults_dir = getenv("FLATTY_DEFAULTS_DIR");
  if (!defaults_dir || *defaults_dir == '\0')
  {
    defaults_dir = FLATTY_DEFAULTS_DIR;
  }

  char *default_config_path = NULL;
  if (asprintf(&default_config_path, "%s/config.conf", defaults_dir) < 0)
    return EXIT_FAILURE;

  const char *config_to_load =
      ctx.cli_config_set ? ctx.config_file : default_config_path;
  if (access(config_to_load, F_OK) == 0)
  {
    if (!config_load(&cfg, config_to_load))
    {
      config_cleanup(&cfg);
      free(default_config_path);
      return EXIT_FAILURE;
    }
  }
  else if (ctx.cli_config_set)
  {
    fprintf(stderr, "Error: Explicit config file not found: %s\n",
            config_to_load);
    config_cleanup(&cfg);
    free(default_config_path);
    return EXIT_FAILURE;
  }
  else
  {
    fprintf(stderr, "Error: Default config not found at %s\n",
            default_config_path);
    config_cleanup(&cfg);
    free(default_config_path);
    return EXIT_FAILURE;
  }
  free(default_config_path);

  if (ctx.cli_output_set)
  {
    free(cfg.output_file);
    cfg.output_file = strdup(ctx.output_file);
    if (!cfg.output_file)
    {
      fprintf(stderr, "Error: Memory allocation failed\n");
      config_cleanup(&cfg);
      return EXIT_FAILURE;
    }
  }

  char *abs_output = NULL;
  if (cfg.output_file[0] == '/')
  {
    abs_output = strdup(cfg.output_file);
  }
  else
  {
    char *cwd = getcwd(NULL, 0);
    if (!cwd || asprintf(&abs_output, "%s/%s", cwd, cfg.output_file) < 0)
    {
      free(cwd);
      fprintf(stderr, "Error: Cannot resolve output path\n");
      config_cleanup(&cfg);
      return EXIT_FAILURE;
    }
    free(cwd);
  }

  FileIdentity excluded[1];
  size_t excluded_count = 0;

  struct stat out_stat;
  if (stat(abs_output, &out_stat) == 0)
  {
    if (!S_ISREG(out_stat.st_mode))
    {
      fprintf(stderr, "Error: Output destination '%s' is not a regular file.\n",
              abs_output);
      free(abs_output);
      config_cleanup(&cfg);
      return EXIT_FAILURE;
    }
    excluded[0].inode = out_stat.st_ino;
    excluded[0].dev = out_stat.st_dev;
    excluded[0].valid = true;
    excluded_count++;
  }

  IgnoreRules rules;
  ignore_rules_init(&rules);

  if (ctx.cli_ignore_set)
  {
    if (!ignore_rules_load_file(&rules, ctx.ignore_file))
    {
      fprintf(stderr, "Error: Cannot load explicit ignore file: %s\n",
              ctx.ignore_file);
      ignore_rules_cleanup(&rules);
      config_cleanup(&cfg);
      free(abs_output);
      return EXIT_FAILURE;
    }
  }
  else
  {
    char *default_ignore_path = NULL;
    if (asprintf(&default_ignore_path, "%s/ignore.conf", defaults_dir) < 0)
    {
      ignore_rules_cleanup(&rules);
      config_cleanup(&cfg);
      free(abs_output);
      return EXIT_FAILURE;
    }

    if (access(default_ignore_path, F_OK) == 0)
    {
      if (!ignore_rules_load_file(&rules, default_ignore_path))
      {
        fprintf(stderr, "Error: Cannot read default ignore rules\n");
        ignore_rules_cleanup(&rules);
        config_cleanup(&cfg);
        free(abs_output);
        free(default_ignore_path);
        return EXIT_FAILURE;
      }
    }
    else
    {
      fprintf(stderr, "Error: Default ignore rules not found at %s\n",
              default_ignore_path);
      ignore_rules_cleanup(&rules);
      config_cleanup(&cfg);
      free(abs_output);
      free(default_ignore_path);
      return EXIT_FAILURE;
    }
    free(default_ignore_path);

    char *project_ignore_path = NULL;
    if (asprintf(&project_ignore_path, "%s/.extractignore", ctx.target_path) <
        0)
    {
      ignore_rules_cleanup(&rules);
      config_cleanup(&cfg);
      free(abs_output);
      return EXIT_FAILURE;
    }
    if (access(project_ignore_path, F_OK) == 0)
    {
      if (!ignore_rules_load_file(&rules, project_ignore_path))
      {
        fprintf(stderr, "Error: Cannot read project .extractignore\n");
        ignore_rules_cleanup(&rules);
        config_cleanup(&cfg);
        free(abs_output);
        free(project_ignore_path);
        return EXIT_FAILURE;
      }
    }
    free(project_ignore_path);
  }

  FileList files;
  file_list_init(&files);

  if (!walk_directory(ctx.target_path, &rules, &files, excluded,
                      excluded_count))
  {
    fprintf(stderr, "Error: Filesystem traversal failed\n");
    file_list_cleanup(&files);
    ignore_rules_cleanup(&rules);
    config_cleanup(&cfg);
    free(abs_output);
    return EXIT_FAILURE;
  }

  WriterContext wctx;
  wctx.root_path = ctx.target_path;
  wctx.output_filename = abs_output;
  wctx.skip_binary_files = cfg.skip_binary_files;

  bool success = write_output(&wctx, &files, cfg.verbose_output);

  if (success)
  {
    printf("Successfully packed %zu files into %s\n", files.count, abs_output);
  }
  else
  {
    fprintf(stderr, "Packing failed.\n");
  }

  file_list_cleanup(&files);
  ignore_rules_cleanup(&rules);
  config_cleanup(&cfg);
  free(abs_output);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}