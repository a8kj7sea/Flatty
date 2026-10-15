#ifndef FLATTY_WRITER_H
#define FLATTY_WRITER_H

#include "core/walker.h"
#include <stdbool.h>

/*
 * @brief Context structure for the output writer.
 */
typedef struct
{
  const char *root_path;
  const char *output_filename;
  bool skip_binary_files;
} WriterContext;

/*
 * @brief Writes the collected file list and their contents to an output file.
 * @param wctx Pointer to the WriterContext containing output settings.
 * @param files Pointer to the FileList of collected files.
 * @param verbose Flag to enable verbose output to stderr.
 * @return true on success, false on write failure.
 */
bool write_output(WriterContext *wctx, FileList *files, bool verbose);

#endif