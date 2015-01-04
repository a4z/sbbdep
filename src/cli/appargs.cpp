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

#include <getopt.h>

namespace sbbdep {

AppArgs::AppArgs()
  : _help(0)
  , _dbname(std::string(std::getenv("HOME") + std::string("/sbbdep.cache")))
  , _query()
  , _outfile()
  , _append_versions(1)
  , _sbbdep_version(0)
  , _nosync(0)
  , _whoneeds(0)
  , _explain_dynlinked(0)
  , _quiet(0)
  , _ldd(0)
  , _featureX(0)
  , _featureXArgs{""}
{
  
}
//--------------------------------------------------------------------------------------------------

AppArgs::~AppArgs()
{
  
}
//--------------------------------------------------------------------------------------------------

void
AppArgs::PrintHelp()
{


  struct Writer
  {
    Writer&
    option( const std::string& option )
    {
      std::cout << std::setw(2) << std::left << "" << std::setw(27) << std::left << option;
      return *this;
    }
    Writer&
    description( const std::string& txt )
    {
      std::cout << std::setw(0) << txt << std::endl;
      return *this;
    }
    Writer&
    descriptionline( const std::string& txt )
    {
      std::cout << std::setw(32) << "" << std::setw(0) << txt << std::endl;
      return *this;
    }
    Writer&
    text( const std::string& txt )
    {
      std::cout << txt << std::endl;
      return *this;
    }    
    Writer&
    newline(  )
    {
      std::cout <<  std::endl;
      return *this;
    }
    
  };

  
  Writer write ;
  write.text("usage: sbbdep [OPTION]... [QUERY]")
   .newline()
   .text(" QUERY can be a single binary/library file")
   .text("              a file from /var/adm/packages")
   .text("              a package DESTDIR of a slackbuild")
   .text(" if QUERY is omitted runs only cache synchronise")
   .text(" if OPTION -c/--cache is omitted, ~/sbbdep.cache will be used")
   .newline()
   .newline() ;
  
  write.text(" available options:") .newline();

  write.option("-c,  --cache") .description("cache file to use")
   .descriptionline("if this option is omitted ")
   .descriptionline("$HOME/sbbdep.cache is used")
   .descriptionline("cache must be read/writable")
   .newline()
   .newline();
  
  write.option("-f,  --file=[FILENAME]") .description("write output to file FILENAME")
   .descriptionline("if this option is omitted ")
   .descriptionline("output will go to stdout")
   .newline()
   .newline();

  write.option("-s,  --short") .description("suppress version information")
    .descriptionline("print only package name without version ")
    .descriptionline("produce a comma separated list of required package names if ")
    .descriptionline("QUERY is a package or DESTDIR")
    .newline()
    .newline();
  
  
  write.option("--nosync") .description("skips synchronise the cache")
   .descriptionline("only useful if cache is up to date")
   .descriptionline("after install/update/remove packages, this option should not be used")
   .descriptionline("if cache is up to date, QUERY will be a bit faster with this option")
   .newline()
   .newline();

  write.option("--quiet") .description("Suppress status information during sync")
   .descriptionline("messages during synchronisation will not be shown")
   .newline()
   .newline();

  
  write.option("--whoneeds") .description("prints packages that depend on arg")
   .descriptionline("instead of printing the requirements of the given QUERY")
   .descriptionline("packages that depend on QUERY are reported")
   .newline()
   .newline();
  
  write.option("--xdl") .description("explain dynamic linked file")
   .descriptionline("reports detailed information about needed and whoneeds")
   .descriptionline("if QUERY is a package than each file will be reported")
   .newline()
   .newline();

  write.option("--ldd") .description("use ldd")
   .descriptionline("per default, sbbdep uses the ELF information from binaries and libraries")
   .descriptionline("with this option, sbbdep executes the ldd command against each dynamic ")
   .descriptionline("linked file and uses the ldd output for dependency resolution")
   .descriptionline("this option is useless for a --whoneeds query")
   .newline()
   .newline();


  write.option("-v,  --version") .description("display sbbdep version")
   .newline()
   .newline();
  
  write.option("-h,  --help") .description("display this text")
   .newline()
   .newline();
  
}
//--------------------------------------------------------------------------------------------------

bool
AppArgs::Parse( int argc, char** argv )
{
  
  bool retVal = true; // default

  if (argc == 1) return true; //sync only

  const char* const short_options = "hf:c:sv";
  

  const struct option long_options[] =
    {
      { "file", required_argument, 0, 1 },
      { "cache", required_argument, 0, 1 },
      { "help", no_argument, &_help, 1 },
      { "short", no_argument, &_append_versions, 0 },
      { "version", no_argument, &_sbbdep_version, 1 },
      { "nosync", no_argument, &_nosync, 1 },
      { "quiet", no_argument, &_quiet, 1 },
      { "whoneeds", no_argument, &_whoneeds, 1 },
      { "xdl", no_argument, &_explain_dynlinked, 1 },
      { "ldd", no_argument, &_ldd, 1 },
      { "fx", optional_argument, 0, 1 }, // undocumented option for the next test...
      { 0, 0, 0, 0 } // Required end   
    };
  
  int optionIdx = 0;
  int optionVal = 0;
  
  while ( optionVal > -1 )
    {
      optionVal = getopt_long(argc, argv, short_options, long_options, &optionIdx);
      std::string optionName;
      switch (optionVal)
        {

        case 1:
          optionName = long_options[optionIdx].name;
          if (optionName == "file")
            _outfile = optarg ? optarg : "";
          else if(optionName == "cache")
            _dbname = optarg ? optarg : "";
          else if(optionName == "fx") {
        	_featureX = 1 ;
        	_featureXArgs = optarg ? optarg : "";
          }

          break;


        case 'c':
          _dbname = optarg ? optarg : "";
          break;

        case 'f':
          _outfile = optarg ? optarg : "";
          break;
          
        case 'h':
          optionVal = -1; //exit from here
          _help = true;
          break;
          
        case 's':
          _append_versions = false;
          break;
          
        case 'v':
          optionVal = -1;
          _sbbdep_version=true;
          break;          
          
        case '?': //unknown param
        case ':': //missing arg
          //std::cerr  << "unknown option -" << optionVal << "\n" ;
          return false;
          
        default:
          break;

        }
      
    }


  // if help was submitted, ignore following cause only helptext is to show
  if( !_help && !_sbbdep_version )
    {
      for (int argidx = optind; argidx < argc; ++argidx)
        {
          if(_query.empty())
            _query = argv[argidx];
          else
            break;
          // TODO, log ignored arguments ....or change _query to a list
        }
    }
  
  return retVal;
  
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
