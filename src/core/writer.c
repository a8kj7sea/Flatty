/*
 * @file writer.c
 * @author a8kj7sea
 * @brief Implementation of the output writer for Flatty.
 *        Writes the directory tree and file contents to a temporary file,
 *        then atomically renames it to the final output file.
 */

#include "core/writer.h"
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define READ_BUFFER_SIZE 8192

/*
 * @brief Prints the directory tree of the collected files to the output stream.
 * @param out The FILE stream to write to.
 * @param files Pointer to the FileList of collected files.
 * @return true on success, false on write failure.
 */
static bool print_tree(FILE *out, FileList *files)
{
  if (fprintf(out, "================================================================================\n") < 0)
    return false;
  if (fprintf(out, "Project Structure\n") < 0)
    return false;
  if (fprintf(out, "================================================================================\n") < 0)
    return false;

  if (files->count == 0)
    return true;

  char prev_path[4096] = "";
  for (size_t i = 0; i < files->count; i++)
  {
    const char *path = files->paths[i];
    const char *p = path;
    const char *prev_p = prev_path;
    int depth = 0;
    const char *last_common_slash = NULL;

    /* Find common directory prefix safely */
    while (*p && *prev_p && *p == *prev_p)
    {
      if (*p == '/')
      {
        depth++;
        last_common_slash = p;
      }
      p++;
      prev_p++;
    }

    /* If mismatch occurred, revert to the last fully matched directory */
    if (*p != *prev_p && (*p != '\0' || *prev_p != '\0'))
    {
      if (last_common_slash)
      {
        p = last_common_slash + 1;
      }
      else
      {
        p = path;
        depth = 0;
      }
    }

    /* Print remaining path components */
    while (*p)
    {
      const char *next_slash = strchr(p, '/');
      if (next_slash)
      {
        if (fprintf(out, "%*s%.*s/\n", depth * 2, "", (int)(next_slash - p), p) < 0)
          return false;
        depth++;
        p = next_slash + 1;
      }
      else
      {
        if (fprintf(out, "%*s%s\n", depth * 2, "", p) < 0)
          return false;
        break;
      }
    }
    snprintf(prev_path, sizeof(prev_path), "%s", path);
  }
  return fprintf(out, "\n") >= 0;
}

/*
 * @brief Copies the content of a single file to the output stream.
 *        Checks for binary files if skip_binary is true.
 * @param out The FILE stream to write to.
 * @param full_path The absolute path of the file to read.
 * @param skip_binary Flag to enable skipping binary files.
 * @param verbose Flag to print verbose messages to stderr.
 * @return true on success, false on read/write failure.
 */
static bool copy_file_content(FILE *out, const char *full_path, bool skip_binary, bool verbose)
{
  struct stat st;
  if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode))
  {
    fprintf(stderr, "Error: File '%s' is not a regular file or cannot be stat'd\n", full_path);
    return false;
  }

  FILE *in = fopen(full_path, "rb");
  if (!in)
  {
    fprintf(stderr, "Error: Cannot open file '%s': %s\n", full_path, strerror(errno));
    return false;
  }

  char buffer[READ_BUFFER_SIZE];
  size_t bytes_read = fread(buffer, 1, READ_BUFFER_SIZE, in);
  if (ferror(in))
  {
    fprintf(stderr, "Error: Reading file '%s' failed\n", full_path);
    fclose(in);
    return false;
  }

  if (skip_binary)
  {
    for (size_t i = 0; i < bytes_read; i++)
    {
      if (buffer[i] == '\0')
      {
        if (verbose)
          fprintf(stderr, "[VERBOSE] Skipping binary file: %s\n", full_path);
        fclose(in);
        if (fprintf(out, "[Binary file skipped]\n") < 0)
          return false;
        return true;
      }
    }
  }

  if (fwrite(buffer, 1, bytes_read, out) != bytes_read)
  {
    fprintf(stderr, "Error: Writing to output failed\n");
    fclose(in);
    return false;
  }

  while ((bytes_read = fread(buffer, 1, READ_BUFFER_SIZE, in)) > 0)
  {
    if (fwrite(buffer, 1, bytes_read, out) != bytes_read)
    {
      fprintf(stderr, "Error: Writing to output failed\n");
      fclose(in);
      return false;
    }
  }

  if (ferror(in))
  {
    fprintf(stderr, "Error: Reading file '%s' failed\n", full_path);
    fclose(in);
    return false;
  }

  fclose(in);
  return true;
}

/*
 * @brief Writes the collected file list and their contents to an output file.
 * @param wctx Pointer to the WriterContext containing output settings.
 * @param files Pointer to the FileList of collected files.
 * @param verbose Flag to enable verbose output to stderr.
 * @return true on success, false on write failure.
 */
bool write_output(WriterContext *wctx, FileList *files, bool verbose)
{
  char *dir_copy = strdup(wctx->output_filename);
  if (!dir_copy)
    return false;

  char *dir = dirname(dir_copy);
  char *temp_template = NULL;
  if (asprintf(&temp_template, "%s/.flatty_XXXXXX", dir) < 0)
  {
    free(dir_copy);
    return false;
  }

  int fd = mkstemp(temp_template);
  if (fd < 0)
  {
    fprintf(stderr, "Error: Cannot create temporary file: %s\n", strerror(errno));
    free(dir_copy);
    free(temp_template);
    return false;
  }

  FILE *out = fdopen(fd, "wb");
  if (!out)
  {
    fprintf(stderr, "Error: Cannot open temporary file stream\n");
    close(fd);
    unlink(temp_template);
    free(dir_copy);
    free(temp_template);
    return false;
  }

  bool success = print_tree(out, files);

  for (size_t i = 0; i < files->count && success; i++)
  {
    const char *rel_path = files->paths[i];
    char *full_path = NULL;
    if (asprintf(&full_path, "%s/%s", wctx->root_path, rel_path) < 0)
    {
      success = false;
      break;
    }

    if (fprintf(out, "================================================================================\n") < 0 ||
        fprintf(out, "File: %s\n", rel_path) < 0 ||
        fprintf(out, "================================================================================\n") < 0)
    {
      success = false;
      free(full_path);
      break;
    }

    if (!copy_file_content(out, full_path, wctx->skip_binary_files, verbose))
    {
      success = false;
    }

    free(full_path);
    if (success && fprintf(out, "\n\n") < 0)
    {
      success = false;
    }
  }

  if (fclose(out) != 0)
  {
    fprintf(stderr, "Error: Closing output file failed\n");
    success = false;
  }

  if (success)
  {
    if (rename(temp_template, wctx->output_filename) != 0)
    {
      fprintf(stderr, "Error: Failed to rename temp file to %s: %s\n", wctx->output_filename, strerror(errno));
      unlink(temp_template);
      success = false;
    }
  }
  else
  {
    unlink(temp_template);
  }

  free(dir_copy);
  free(temp_template);
  return success;
}