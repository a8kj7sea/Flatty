#ifndef FLATTY_CONFIG_LOADER_H
#define FLATTY_CONFIG_LOADER_H

#include <stdbool.h>

/*
 * @brief Configuration structure for Flatty.
 *        Holds settings loaded from config files or CLI arguments.
 */
typedef struct
{
  char *output_file;
  bool skip_binary_files;
  bool verbose_output;
} Config;

/*
 * @brief Initializes the Config struct with default values.
 * @param cfg Pointer to the Config struct to initialize.
 */
void config_init(Config *cfg);

/*
 * @brief Loads configuration from a specified file.
 * @param cfg Pointer to the Config struct to populate.
 * @param file_path Path to the configuration file.
 * @return true on success, false on failure.
 */
bool config_load(Config *cfg, const char *file_path);

/*
 * @brief Cleans up resources allocated by the Config struct.
 * @param cfg Pointer to the Config struct to clean up.
 */
void config_cleanup(Config *cfg);

#endif