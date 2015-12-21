/* Return the basename of a pathname.
   This file is in the public domain. */

/*
NAME
	basename -- return pointer to last component of a pathname

SYNOPSIS
	char *basename (const char *name)

DESCRIPTION
	Given a pointer to a string containing a typical pathname
	(/usr/src/cmd/ls/ls.c for example), returns a pointer to the
	last component of the pathname ("ls.c" in this case).

BUGS
	Presumes a UNIX or DOS/Windows style path with UNIX or DOS/Windows 
	style separators.
*/

#include <ctype.h>
#include <string.h>
#include "basename.h"

char *basename(const char *name)
{
  const char *base;

  /* Skip over the disk name in MSDOS pathnames. */
  if (isalpha (name[0]) && name[1] == ':') 
    name += 2;

  for (base = name; *name; name++)
      if (*name == '/' || *name == '\\')
        base = name + 1;
  return (char *) base;
}

char *dirname(const char *name)
{
  char *dir = strdup(name);

  if(strrchr(dir, '/') != NULL)
    *(strrchr(dir, '/')) = '\0';
  else if(strrchr(dir, '\\') != NULL)
    *(strrchr(dir, '\\')) = '\0';
  if(!strcmp(dir, name))
    return "";
  return dir;
}
