/*
--------------Copyright (c) 2010-2015 H a r a l d  A c h i t z---------------
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


#include "appargs.hpp"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <boost/tokenizer.hpp>

#include <getopt.h>

namespace sbbdep {


  namespace
  {
    std::vector<std::string>
    ignoreList(const std::string& list)
    {
      using namespace std;

      if (list.empty())
        return vector<string>() ;

      using tokenizer = boost::tokenizer<boost::char_separator<char>>;
      boost::char_separator<char> sep(", ");
      tokenizer tokens(list, sep);

      vector<string> vec {begin (tokens), end (tokens)} ;
      sort (begin (vec), end (vec));

      return vec ;
    }

  }



AppArgs::AppArgs()
  : _help(0)
  , _dbname(std::string(std::getenv("HOME") + std::string("/sbbdep.cache")))
  , _query()
  , _outfile()
  , _short_names(0)
  , _sbbdep_version(0)
  , _nosync(0)
  , _require(0)
  , _whoneeds(0)
  , _explain_dynlinked(0)
  , _quiet(0)
  , _ldd(0)
  , _lookup(0)
  , _featureX(0)
  , _featureXArgs{""}
  , _varAdmDir{"/var/adm/packages"}
  , _bdtree(0)
{
  
}
//------------------------------------------------------------------------------

AppArgs::~AppArgs()
{
  
}
//------------------------------------------------------------------------------

void
AppArgs::printHelp()
{

std::cout << R"~(
 a tool to query binary dependencies of files from installed packages. 

usage: sbbdep [OPTION]... [QUERY]

 QUERY can be a single binary/library file
              a file from /var/adm/packages
              a package DESTDIR of a slackbuild
              some existing file

 If QUERY is omitted sbbdep runs only the cache synchronisation.
 The default behavior is to find dependencies of QUERY.
 If QUERY is not a binary file or an installed package but a valid file name
 sbbdep will search for the file in the package database. 

 available options:

  -c, --cache=[FILENAME] 
      Cache file to use. The cache file must be read/writable.
      If this option is omitted $HOME/sbbdep.cache is used.  
                            
  -f, --file=[FILENAME]
      write output to file FILENAME
      If this option is omitted the output will go to stdout.   

  -s, --short 
      Report package name without version 

  --nosync 
      Skip synchronise the cache. Only useful if cache is up to date.
      If the cache is out of date wrong results might be reported.    

  --quiet
      Suppress status information during sync

  --require
      this is the default option if not --whoneeds is used.
      If this option is explicit given and QUERY is not a binary file 
      the search in the package database will be skipped.  

  --whoneeds
      Instead of printing the requirements of the given QUERY packages that 
      depend on QUERY are reported.

  --xdl  
      explain dynamic linked file. Reports detailed information,
      works with require and whoneeds queries.

  --ldd
     You do not want to use this option to obtain dependency information.
     Using this option sbbdep will use the ldd command.
     This will not only print direct requirements of a given file but also pull
     in the dependencies of the requirements. 
     This option is useless for a --whoneeds query.

  --bdtree
     Prints a hierarchical dependency tree of QUERY.
     This can produce a quite long output of for some binaries or libraries and
     a very very big long for some package.

  --ignore
     Takes a comma separated list of package names, either the base name or
     the full package name, to ignore in the output.
     aaa_base,cxxlibs might be an example for this option.
     This option is valid for dependency queries, useless otherwise.

  -l, --lookup
     Search for QUERY in the package database and skip all other operations
                             
  --admdir=[DIRNAME]
     Use DIRNAME as package database. Default is /var/adm/packages 
     Usually you do not want to use this option                         

  -v,  --version
     display sbbdep version

  -h,  --help
     display this text

For more information visit the sbbdep wiki.
               https://bitbucket.org/a4z/sbbdep/wiki/Home

)~";


}
//------------------------------------------------------------------------------

bool
AppArgs::parse( int argc, char** argv )
{
  
  bool retVal = true; // default

  if (argc == 1)
    { //sync only
      return true;
    }

  const char* const short_options = "hlf:c:sv";

  const struct option long_options[] =
    {
      { "file", required_argument, 0, 1 },
      { "cache", required_argument, 0, 1 },
      { "help", no_argument, &_help, 1 },
      { "short", no_argument, &_short_names, 1 },
      { "version", no_argument, &_sbbdep_version, 1 },
      { "nosync", no_argument, &_nosync, 1 },
      { "quiet", no_argument, &_quiet, 1 },
      { "whoneeds", no_argument, &_whoneeds, 1 },
      { "require", no_argument, &_require, 1 },
      { "xdl", no_argument, &_explain_dynlinked, 1 },
      { "ldd", no_argument, &_ldd, 1 },
      { "lookup", no_argument, &_lookup, 1 },
      { "fx", optional_argument, 0, 1 }, // undocumented option for  test...
      { "admdir", required_argument, 0, 1 },
      { "bdtree", no_argument, &_bdtree, 1 },
      { "ignore", required_argument, 0, 1 },
      { 0, 0, 0, 0 } // Required end   
    };
  
  int optionIdx = 0;
  int optionVal = 0;

  while (optionVal > -1)
    {
      optionVal = getopt_long (argc, argv,
                               short_options,
                               long_options,
                               &optionIdx);
      std::string optionName;
      switch (optionVal)
        {

        case 1 :
          optionName = long_options [optionIdx].name;
          if (optionName == "file")
            {
              _outfile = optarg ? optarg : "";
            }
          else if (optionName == "cache")
            {
              _dbname = optarg ? optarg : "";
            }
          else if (optionName == "fx")
            {
              _featureX = 1;
              _featureXArgs = optarg ? optarg : "";
            }
          else if (optionName == "admdir")
            {
              _varAdmDir = optarg ? optarg : "";
            }
          else if (optionName == "ignore")
            {
              const std::string l = optarg ? optarg : "";
              if (not l.empty ())
                _ignore  = ignoreList (l) ;
            }
          break;

        case 'c' :
          _dbname = optarg ? optarg : "";
          break;

        case 'f' :
          _outfile = optarg ? optarg : "";
          break;

        case 'h' :
          optionVal = -1; //exit from here
          _help = true;
          break;

        case 'l' :
          _lookup = true;
          break;

        case 's' :
          _short_names = true;
          break;

        case 'v' :
          optionVal = -1;
          _sbbdep_version = true;
          break;

        case '?' : //unknown param
        case ':' : //missing arg
          //std::cerr  << "unknown option -" << optionVal << "\n" ;
          return false;

        default :
          break;

        }

    }

  // if help was submitted, ignore following cause only helptext is to show
  if (!_help && !_sbbdep_version)
    {
      std::vector<std::string> ignores;
      for (int argidx = optind; argidx < argc; ++argidx)
        {
          if (_query.empty ())
            {
              _query = argv [argidx];
            }
          else
            {
              ignores.push_back (argv [argidx]);
            }
        }
      if (not ignores.empty ())
        {
          std::cerr << "too much args, ignore";
          for (auto& ignore : ignores)
            {
              std::cerr << " " << ignore;
            }
          std::cerr << "\n handle only query: " << _query << "\n\n";
        }

    }
  
  return retVal;
  
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}
