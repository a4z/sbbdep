/*
 --------------Copyright (c) 2012-2018 H a r a l d  A c h i t z---------------
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

#include "report.hpp"

#include <sbbdep/dircontent.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pathname.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/utils/concurrentpeek.hpp>

#include <future>
#include <fstream>
#include <thread>
#include <vector>


namespace sbbdep {
namespace cli {

namespace {

// check pkgfile line for line if search exist
bool
process (const std::string& pkgfile, const Path& search)
{

  std::ifstream ins (pkgfile.c_str ());
  if (!ins.good ())
    {
      LogError () << "Waring: can not read " << pkgfile ;
      return false;
    }


  // helper to check if search exists in given line
  // and respecting some specials pkg-files can contain
  auto file_name_match_in =
      [&search](const std::string& line) -> bool
        {

          std::string filename = "/"+ search.base();
          if (line.find( filename,
                        line.size ()-filename.size ()) != std::string::npos)
            {
              return true;
            }


          if((line.find( ".new" , line.size()-4 ) != std::string::npos))
            {
              if ( line.find( filename.c_str() ,
                              line.size()-filename.size() -4,
                              filename.size() ) != std::string::npos)
              return true;
            }

          return false;
        }; //----------------------------------

  // helper to check if found name is exact match
  auto path_is_same_or_incoming = [&search](const Path match)-> bool
    {
      if (search.dir () == match.dir ())
        {
          return true;
        }

      PathName p (match.dir ());
      if( p.base ()=="incoming" && p.dir () == search.dir ())
        {
          return true;
        }

      return false;
    }; //----------------------------------


  std::string line;
  int cnt = 0; // for skipping the first lines..
  while (not ins.eof ())
    {
      std::getline (ins, line);
      //skip the first lines in file, they contain no info for the search
      if (++cnt < 6)
        {
          continue;
        }

      if (file_name_match_in (line))
        {
          PathName match ("/" + line);

          if (path_is_same_or_incoming (match))
            {
              LogMsg () << "absolute match in "
                  << pkgfile  << ": " << search  << "\n"
                  << "    -> " << line   ;
            }

          else
            {
              LogMsg () << " filename found in " << pkgfile << ": "
                  << search.base () << "\n" << " -> line was :" << line ;
            }


          // if there was a match, no need to proceed
          return true;
        }
    }
  return false;
}
//------------------------------------------------------------------------------

std::vector<std::string>
VarAdmPkgNames()
{
  auto admdir = pkgAdmDir();
  std::vector<std::string> retval;

  admdir.forEach ([&retval](const std::string& d,const std::string& f) -> bool
    {
      retval.push_back (d+"/"+f);
      return true;
    });

  return retval ;
}
//------------------------------------------------------------------------------

} // ns

// _findInPackages

bool
fileInPackages (const sbbdep::Path& filepath)
{

  ConcurrentPeek<std::string> pkglist (VarAdmPkgNames ()) ;
  auto search = [&pkglist, &filepath] () -> bool
    {
      bool hadMatch = false ;
      std::string pkgfile = pkglist.pop ();
      while(not pkgfile.empty  ())
        {
          if (process (pkgfile, filepath))
            {
              hadMatch = true;
            }
          pkgfile = pkglist.pop ();
        }
      return hadMatch ;
    };


  auto match1 = std::async (std::launch::async, search) ;
  auto match2 = search () ;

  if( not match2 and not match1.get())
    {
      LogInfo() << "nothing found\n" ;
    }

  LogInfo() << "" ; // just make a new line

  return true;
}
//------------------------------------------------------------------------------


void
lookupInPackages(const std::string& what)
{

  ConcurrentPeek<std::string> pkglist (VarAdmPkgNames ()) ;

  auto search = [&pkglist, &what] ()
    {
      std::string pkgfile = pkglist.pop ();
      while(not pkgfile.empty  ())
        {
          // collet all info in this pkg to write it at onece at the end
          std::ifstream ins (pkgfile.c_str ());
          if (!ins.good ())
            {
              LogError () << "Waring: can not read " << pkgfile ;
            }
          else
            {
              int cnt = 0 ;
              std::string results ;
              while (not ins.eof ())
                {
                  std::string line;
                  std::getline (ins, line);
                  //first lines in file,  contain no info for the search
                  if (++cnt < 6)
                    {
                      continue;
                    }

                  if(line.find (what) != std::string::npos)
                    {
                      results += "\n " +
                          PathName (pkgfile).base () + ": "  + line ;
                    }
                }
              if (not results.empty ())
                {
                  LogMsg () << results ;
                }
            }

          pkgfile = pkglist.pop ();
        }
    };

  std::thread t1(search) ;
  search() ;
  if(t1.joinable())
    {
      t1.join();
    };

  LogInfo() << "" ; // just make a new line

}




}
} // ns
