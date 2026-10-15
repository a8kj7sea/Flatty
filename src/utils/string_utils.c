/*
 * @file string_utils.c
 * @author a8kj7sea
 * @brief Implementation of string manipulation utilities.
 */

#include "utils/string_utils.h"
#include <stddef.h>
#include <string.h>

/*
 * @brief Trims leading and trailing whitespace from a string.
 * @param str The string to trim. The string is modified in place.
 * @return Pointer to the modified string.
 */
char *string_utils_trim(char *str)
{
  if (!str)
    return NULL;

  char *start = str;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
  {
    start++;
  }

  if (*start == '\0')
  {
    *str = '\0';
    return str;
  }

  char *end = start + strlen(start) - 1;
  while (end > start &&
         (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
  {
    end--;
  }
  *(end + 1) = '\0';

  if (start != str)
  {
    memmove(str, start, end - start + 2);
  }

  return str;
}