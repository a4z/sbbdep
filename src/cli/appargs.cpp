/*
--------------Copyright (c) 2010-2012 H a r a l d  A c h i t z---------------
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

#include <getopt.h>

#include <iostream>
#include <iomanip>

namespace sbbdep {

AppArgs::AppArgs() :
  m_help(false), m_dbname(), m_query(), m_outfile(), m_append_versions(true), 
  m_sbbdep_version(false), m_nosync(false),m_whoneeds(false), m_explain_dynlinked(false)
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
  
  
  
  Writer write;
  write.text("usage: sbbdep [OPTION]... [QUERY]") ;
  write.newline();
  write.text(" QUERY can be a single binary/library file");
  write.text("              a file from /var/adm/packages");
  write.text("              a package DESTDIR of a slackbuild");
  write.text(" if QUERY is omitted runs only cache synchronise");
  write.newline();
  write.text(" if OPTION -c/--cache is omitted, ~/sbbdep.cache will be used").newline() ;
  write.newline();
  
  write.text(" available options:") .newline();

  write.option("-c,  --cache") .description("cache file to use") ; 
  write.descriptionline("if this option is omitted ") ;
  write.descriptionline("$HOME/sbbdep.cache is used") ;
  write.descriptionline("cache must be read/writable") ;
  write.newline();
  write.newline();  
  
  write.option("-f,  --file=[FILENAME]") .description("write output to file FILENAME") ;
  write.descriptionline("if this option is omitted ") ;
  write.descriptionline("output will go to stdout") ;  
  write.newline();
  write.newline();

  write.option("-s,  --short") .description("suppress version information") ; 
  write.descriptionline("produce a comma separated list of required package names ") ;
  write.newline();
  write.newline();  
  
  
  write.option("--nosync") .description("skips synchronise the cache") ;
  write.descriptionline("only usefull if cache is up to date") ;
  write.descriptionline("after install/update/remove packages, this option should not be used") ;
  write.descriptionline("if cache is up to date, QUERY will be a bit faster with this option") ;
  
  write.newline();
  write.newline();  
  
  write.option("--whoneeds") .description("prints packages that depend on arg") ;
  write.descriptionline("instead of printing the requirements of the given arg") ;
  write.descriptionline("packages that depend on the given arg are written to std::out") ;
  write.newline();
  write.newline();  
  
  write.option("--xdl") .description("explain dynamic linked file") ;
  write.descriptionline("needs a dynamic linked binary as QUERY") ;
  write.descriptionline("reports detailed information about needed and whoneeds") ;
  write.newline();
  write.newline();

  write.option("-v,  --version") .description("display sbbdep version") ;
  write.newline();
  write.newline();  
  
  write.option("-h,  --help") .description("display this text") ;
  write.newline();
  write.newline();  
  
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
      { "help", no_argument, 0, 'h' },
      { "file", required_argument, 0, 'f' },
      { "cache", required_argument, 0, 'c' },
      { "short", required_argument, 0, 's' },
      { "version", no_argument, 0, 'v' },         
      { "nosync", no_argument, 0, 1 },
      { "whoneeds", no_argument, 0, 1 },
      { "xdl", no_argument, 0, 1 },
      { 0, 0, 0, 0 } // Required end   
    };
  
  int optionIdx = 0;
  int optionVal = 0;
  
  while (!(optionVal < 0))
    {
      optionVal = getopt_long(argc, argv, short_options, long_options, &optionIdx);
      std::string optionName;
      
      switch (optionVal)
        {
        case 1:
          optionName = long_options[optionIdx].name;
          if (optionName == "nosync") m_nosync = true;
          else if(optionName == "whoneeds") m_whoneeds = true;
          else if (optionName == "xdl") m_explain_dynlinked = true;
          break;
          
        case 'c':
          m_dbname = optarg ? optarg : "";
          break;
          
        case 'h':
          optionVal = -1; //exit from here
          m_help = true;
          break;
          
        case 'f':
          m_outfile = optarg ? optarg : "";
          break;
          
          
        case 's':
          m_append_versions = false;
          break;
          
        case 'v':
          optionVal = -1;
          m_sbbdep_version=true;
          break;          
          
        case '?': //unknown param
        case ':': //missing arg
          std::cout << "\n   commandline parsing failed, execution stopped\n" << std::endl;
          optionVal = -1;
          retVal = false;
          break;
          
        default:
          break; // throw never reach ? 

        }
      
    }

  // if help was submitted, ignore following cause only helptext is to show
  if (!m_help && !m_sbbdep_version)
    { // use extern optind to find out if one additional argument was given use as file?    
      if (argc - optind != 1)
        {
          std::cerr << " wrong arguments " << std::endl; 
          retVal = false;
        }
      else
        { //std::cout << " use " << argv[optind] << " as f arg" << std::endl ;
          m_query = argv[optind];
        }
    }
  
  return retVal;
  
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
