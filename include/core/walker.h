#ifndef FLATTY_WALKER_H
#define FLATTY_WALKER_H

#include "ignore/ignore_parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/*
 * @brief Dynamic list of file paths collected during traversal.
 */
typedef struct
{
  char **paths;
  size_t count;
  size_t capacity;
} FileList;

/*
 * @brief Structure to uniquely identify a file by its inode and device.
 */
typedef struct
{
  ino_t inode;
  dev_t dev;
  bool valid;
} FileIdentity;

/*
 * @brief Initializes the FileList structure.
 * @param list Pointer to the FileList to initialize.
 */
void file_list_init(FileList *list);

/*
 * @brief Cleans up memory allocated for the FileList.
 * @param list Pointer to the FileList to clean up.
 */
void file_list_cleanup(FileList *list);

/*
 * @brief Adds a file path to the FileList.
 * @param list Pointer to the FileList.
 * @param path The file path to add.
 * @return true on success, false on allocation failure.
 */
bool file_list_add(FileList *list, const char *path);

/*
 * @brief Walks a directory recursively, collecting regular files that are not ignored.
 * @param root_path The root directory to start traversal from.
 * @param rules Ignore rules to apply during traversal.
 * @param out_list Pointer to the FileList to populate with collected paths.
 * @param excluded Array of FileIdentity to exclude from the list.
 * @param excluded_count Number of identities in the excluded array.
 * @return true on success, false on traversal failure.
 */
bool walk_directory(const char *root_path, IgnoreRules *rules,
                    FileList *out_list, FileIdentity *excluded,
                    size_t excluded_count);

#endif