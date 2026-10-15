/*
 * @file walker.c
 * @author a8kj7sea
 * @brief Filesystem traversal implementation using nftw.
 *        Walks directories, applies ignore rules, and collects regular files.
 */

#include "core/walker.h"
#include "utils/string_utils.h"
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static FileList *g_file_list = NULL;
static IgnoreRules *g_rules = NULL;
static char *g_root_path = NULL;
static size_t g_root_len = 0;
static FileIdentity g_excluded[2];
static size_t g_excluded_count = 0;
static bool g_walk_error = false;

/*
 * @brief Initializes the FileList structure.
 * @param list Pointer to the FileList to initialize.
 */
void file_list_init(FileList *list)
{
  list->paths = NULL;
  list->count = 0;
  list->capacity = 0;
}

/*
 * @brief Cleans up memory allocated for the FileList.
 * @param list Pointer to the FileList to clean up.
 */
void file_list_cleanup(FileList *list)
{
  if (!list->paths)
    return;
  for (size_t i = 0; i < list->count; i++)
  {
    free(list->paths[i]);
  }
  free(list->paths);
  file_list_init(list);
}

/*
 * @brief Adds a file path to the FileList.
 * @param list Pointer to the FileList.
 * @param path The file path to add.
 * @return true on success, false on allocation failure.
 */
bool file_list_add(FileList *list, const char *path)
{
  if (list->count == list->capacity)
  {
    size_t new_cap = list->capacity == 0 ? 16 : list->capacity * 2;
    char **new_paths = realloc(list->paths, new_cap * sizeof(char *));
    if (!new_paths)
      return false;
    list->paths = new_paths;
    list->capacity = new_cap;
  }
  list->paths[list->count] = strdup(path);
  if (!list->paths[list->count])
    return false;
  list->count++;
  return true;
}

/*
 * @brief Callback function for nftw.
 *        Determines if a file should be included in the FileList.
 * @param fpath The full path of the current file.
 * @param sb Pointer to the stat structure for the file.
 * @param typeflag The type of the file (FTW_F, FTW_D, etc.).
 * @param ftwbuf Pointer to the FTW structure.
 * @return FTW_CONTINUE to continue traversal, FTW_SKIP_SUBTREE to skip a directory, or FTW_STOP on error.
 */
static int nftw_callback(const char *fpath, const struct stat *sb, int typeflag,
                         struct FTW *ftwbuf)
{
  (void)ftwbuf;

  if (typeflag == FTW_NS || typeflag == FTW_DNR)
  {
    fprintf(stderr, "Error: Cannot access %s: %s\n", fpath, strerror(errno));
    g_walk_error = true;
    return FTW_STOP;
  }

  if (S_ISLNK(sb->st_mode))
    return FTW_CONTINUE;

  const char *rel_path = fpath + g_root_len;
  if (*rel_path == '/')
    rel_path++;
  if (*rel_path == '\0')
    return FTW_CONTINUE;

  bool is_dir = S_ISDIR(sb->st_mode);

  for (size_t i = 0; i < g_excluded_count; i++)
  {
    if (g_excluded[i].valid && sb->st_ino == g_excluded[i].inode &&
        sb->st_dev == g_excluded[i].dev)
    {
      return FTW_CONTINUE;
    }
  }

  if (ignore_matches(g_rules, rel_path, is_dir))
  {
    if (is_dir)
      return FTW_SKIP_SUBTREE;
    return FTW_CONTINUE;
  }

  if (is_dir)
    return FTW_CONTINUE;

  if (!S_ISREG(sb->st_mode))
  {
    return FTW_CONTINUE; /* Skip special files (FIFOs, devices) */
  }

  if (!file_list_add(g_file_list, rel_path))
  {
    fprintf(stderr, "Error: Memory allocation failed\n");
    g_walk_error = true;
    return FTW_STOP;
  }

  return FTW_CONTINUE;
}

/*
 * @brief Comparison function for qsort to sort paths alphabetically.
 * @param a Pointer to the first path string.
 * @param b Pointer to the second path string.
 * @return Less than, equal to, or greater than zero.
 */
static int compare_paths(const void *a, const void *b)
{
  return strcmp(*(const char **)a, *(const char **)b);
}

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
                    size_t excluded_count)
{
  g_file_list = out_list;
  g_rules = rules;
  g_excluded_count = excluded_count;
  for (size_t i = 0; i < excluded_count; i++)
    g_excluded[i] = excluded[i];
  g_walk_error = false;

  char *abs_root = realpath(root_path, NULL);
  if (!abs_root)
  {
    fprintf(stderr, "Error: Cannot resolve path '%s': %s\n", root_path,
            strerror(errno));
    return false;
  }
  g_root_path = abs_root;
  g_root_len = strlen(abs_root);

  int ret = nftw(abs_root, nftw_callback, 32, FTW_PHYS | FTW_ACTIONRETVAL);

  free(abs_root);
  g_root_path = NULL;

  if (ret != 0 || g_walk_error)
  {
    return false;
  }

  if (out_list->count > 0)
  {
    qsort(out_list->paths, out_list->count, sizeof(char *), compare_paths);
  }
  return true;
}