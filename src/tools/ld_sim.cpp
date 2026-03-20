/*
--------------Copyright (c) 2010-2026 H a r a l d  A c h i t z---------------
-----------< h a r a l d dot a c h i t z at g m a i l dot c o m >------------
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <climits>
#include <exception>
#include <iostream>

using namespace std;

struct DlOpen
{
  DlOpen (std::string f, int flags = RTLD_LAZY | RTLD_LOCAL)
  : obj{std::move (f)}
  {
    (void)dlerror (); // clear all
    handle = dlmopen (LM_ID_NEWLM, obj.c_str (), flags);
    if (!handle)
      {
        const std::string msg = "DlOpen " + obj + ": " + dlerror ();
        throw std::runtime_error{msg};
      }
  }

  ~DlOpen ()
  {
    if (handle)
      dlclose (handle);
  }

  DlOpen (const DlOpen&) = delete;
  DlOpen (DlOpen&& other)
  : obj{std::move (other.obj)}
  , handle{other.handle}
  {
    other.handle = nullptr;
  };

  DlOpen& operator= (const DlOpen&) = delete;
  DlOpen& operator= (DlOpen&&)      = delete;

  std::string obj;
  void*       handle{nullptr};
};

struct DlOpenUser
{
  DlOpenUser (DlOpen& dlo)
  {
    if (dlo.handle == nullptr)
      {
        const std::string msg = "DlLocation, dlo with nullptr handle";

        throw std::runtime_error{msg};
      }

    (void)dlerror (); // clear all
  }
};

struct DlLocation : DlOpenUser
{
  DlLocation (DlOpen& dlo)
  : DlOpenUser{dlo}
  {
    char path[PATH_MAX] = {0};

    if (dlinfo (dlo.handle, RTLD_DI_ORIGIN, &path[0]) == -1)
      {
        const std::string msg = "DlLocation " + dlo.obj + ": " + dlerror ();

        throw std::runtime_error{msg};
      }

    dir = path;
  }

  std::string dir{};
};

struct DlSerInfo : DlOpenUser
{
  DlSerInfo (DlOpen& dlo)
  : DlOpenUser{dlo}
  {
  }
};

int
main (int argc, char** argv)
{
  if (argc < 2)
    {
      cerr << "argument missing\n";
      return EXIT_FAILURE;
    }

  DlOpen     dlo{argv[1]};
  DlLocation dll{dlo};

  cout << dlo.obj << " found in " << dll.dir << endl;

#ifdef IGNORE_THIS_

  void* handle = dlmopen (LM_ID_NEWLM, argv[1], RTLD_LAZY | RTLD_LOCAL);

  if (!handle)
    {
      // fprintf(stderr, "no handle %s\n", dlerror());
      cerr << "no handle for ' " << argv[1] << " ', " << dlerror () << "\n";

      exit (EXIT_FAILURE);
    }

  Dl_serinfo  serinfo;
  Dl_serinfo* sip;

  /* Discover the size of the buffer that we must pass to
      RTLD_DI_SERINFO */

  if (dlinfo (handle, RTLD_DI_SERINFOSIZE, &serinfo) == -1)
    {
      fprintf (stderr, "RTLD_DI_SERINFOSIZE failed: %s\n", dlerror ());
      exit (EXIT_FAILURE);
    }

  /* Allocate the buffer for use with RTLD_DI_SERINFO */

  sip = (Dl_serinfo*)malloc (serinfo.dls_size);
  if (sip == NULL)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }

  /* Initialize the 'dls_size' and 'dls_cnt' fields in the newly
     allocated buffer */

  if (dlinfo (handle, RTLD_DI_SERINFOSIZE, sip) == -1)
    {
      fprintf (stderr, "RTLD_DI_SERINFOSIZE failed: %s\n", dlerror ());
      exit (EXIT_FAILURE);
    }

  /* Fetch and print library search list */

  if (dlinfo (handle, RTLD_DI_SERINFO, sip) == -1)
    {
      fprintf (stderr, "RTLD_DI_SERINFO failed: %s\n", dlerror ());
      exit (EXIT_FAILURE);
    }

  for (size_t j = 0; j < serinfo.dls_cnt; j++)
    printf ("dls_serpath[%d].dls_name = %s\n",
            (int)j,
            sip->dls_serpath[j].dls_name);

  free (sip);

  char path[PATH_MAX] = {0};

  if (dlinfo (handle, RTLD_DI_ORIGIN, &path[0]) == -1)
    {
      fprintf (stderr, "RTLD_DI_ORIGIN failed: %s\n", dlerror ());
      exit (EXIT_FAILURE);
    }

  cout << "Found in: " << path << endl;

  dlclose (handle);

#endif

  return EXIT_SUCCESS;
}
