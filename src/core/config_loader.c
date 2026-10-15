/*
 * @file config_loader.c
 * @author a8kj7sea
 * @brief Implementation of the configuration loader for Flatty.
 *        Parses key=value pairs from a file and populates the Config struct.
 */

#include "core/config_loader.h"
#include "utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * @brief Initializes the Config struct with default values.
 * @param cfg Pointer to the Config struct to initialize.
 */
void config_init(Config *cfg)
{
  cfg->output_file = strdup("project_content.txt");
  cfg->skip_binary_files = true;
  cfg->verbose_output = false;
}

/*
 * @brief Cleans up resources allocated by the Config struct.
 * @param cfg Pointer to the Config struct to clean up.
 */
void config_cleanup(Config *cfg)
{
  if (cfg->output_file)
  {
    free(cfg->output_file);
    cfg->output_file = NULL;
  }
}

/*
 * @brief Parses a boolean value from a string.
 * @param value The string to parse.
 * @param out Pointer to store the parsed boolean value.
 * @return true if parsing was successful, false otherwise.
 */
static bool parse_bool(const char *value, bool *out)
{
  if (strcmp(value, "true") == 0)
  {
    *out = true;
    return true;
  }
  else if (strcmp(value, "false") == 0)
  {
    *out = false;
    return true;
  }
  return false;
}

/*
 * @brief Loads configuration from a specified file.
 * @param cfg Pointer to the Config struct to populate.
 * @param file_path Path to the configuration file.
 * @return true on success, false on failure.
 */
bool config_load(Config *cfg, const char *file_path)
{
  FILE *file = fopen(file_path, "r");
  if (!file)
  {
    fprintf(stderr, "Error: Cannot open config file: %s\n", file_path);
    return false;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int line_num = 0;

  while ((read = getline(&line, &len, file)) != -1)
  {
    line_num++;
    char *trimmed = string_utils_trim(line);
    if (*trimmed == '\0' || *trimmed == '#')
      continue;

    char *eq = strchr(trimmed, '=');
    if (!eq)
    {
      fprintf(stderr, "Error in %s line %d: Missing '='\n", file_path, line_num);
      free(line);
      fclose(file);
      return false;
    }

    *eq = '\0';
    char *key = string_utils_trim(trimmed);
    char *value = string_utils_trim(eq + 1);

    if (*key == '\0' || *value == '\0')
    {
      fprintf(stderr, "Error in %s line %d: Empty key or value\n", file_path, line_num);
      free(line);
      fclose(file);
      return false;
    }

    bool bool_val;
    if (strcmp(key, "output_file") == 0)
    {
      free(cfg->output_file);
      cfg->output_file = strdup(value);
      if (!cfg->output_file)
      {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(line);
        fclose(file);
        return false;
      }
    }
    else if (strcmp(key, "skip_binary_files") == 0)
    {
      if (!parse_bool(value, &bool_val))
      {
        fprintf(stderr, "Error in %s line %d: Invalid boolean value\n", file_path, line_num);
        free(line);
        fclose(file);
        return false;
      }
      cfg->skip_binary_files = bool_val;
    }
    else if (strcmp(key, "verbose_output") == 0)
    {
      if (!parse_bool(value, &bool_val))
      {
        fprintf(stderr, "Error in %s line %d: Invalid boolean value\n", file_path, line_num);
        free(line);
        fclose(file);
        return false;
      }
      cfg->verbose_output = bool_val;
    }
    else
    {
      fprintf(stderr, "Error in %s line %d: Unknown key '%s'\n", file_path, line_num, key);
      free(line);
      fclose(file);
      return false;
    }
  }

  if (ferror(file))
  {
    fprintf(stderr, "Error: Reading config file failed\n");
    free(line);
    fclose(file);
    return false;
  }

  free(line);
  fclose(file);
  return true;
}