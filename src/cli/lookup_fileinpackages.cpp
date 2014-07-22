/*
 --------------Copyright (c) 2012-2014 H a r a l d  A c h i t z---------------
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

#include "lookup_fileinpackages.hpp"

#include <sbbdep/pkgadmdir.hpp>
#include <sbbdep/pathname.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/log.hpp>

#include <fstream>
#include <vector>


using namespace sbbdep;

namespace sbbdep {
namespace lookup {

namespace {

  // check pkgfile line for line if search exist
  void process(std::string&& pkgfile, const Path& search)
  {

    std::ifstream ins( pkgfile.c_str() );
    if ( !ins.good()) return ; //false // TODO ,err message whatever;

   // helper to check if search exists in given line respecting some specials pkg-files can contain
    auto file_name_match_in = [&search](const std::string& line) -> bool {

      std::string filename = "/"+ search.getBase();
      if(line.find( filename , line.size()-filename.size() ) != std::string::npos)
        return true;

      if((line.find( ".new" , line.size()-4 ) != std::string::npos)){
          if ( line.find( filename.c_str() , line.size()-filename.size() -4, filename.size() ) != std::string::npos)
            return true ;
      }

      return false;
    };//----------------------------------

   // helper to check if found name is exact match
    auto path_is_same_or_incoming = [&search](const Path match)-> bool {
      if (search.getDir() == match.getDir())
        return true;

      PathName p(match.getDir());
      if( p.getBase()=="incoming" &&  p.getDir() == search.getDir() )
        return true;

      return false ;
    };//----------------------------------

    std::string line;
    int cnt = 0; // for skipping the first lines..
    while( not ins.eof() )
      {
        std::getline(ins, line);
        //skip the first lines in file, they contain no info for the search
        if ( ++cnt < 6)
          continue;

        if( file_name_match_in(line) )
          {
            PathName match("/"+line);

            if (path_is_same_or_incoming(match))
              WriteAppMsg()  << "absolute match in " << pkgfile << ": " << search  << "\n";
            else
              WriteAppMsg()  << " filename found in " << pkgfile << ": " << search.getBase() << "\n"
                << " -> line was :" << line << "\n";

            // if there was a match, no need to proceed
            return ;
          }
      }

  }

} // ns


// _findInPackages

bool fileInPackages(const sbbdep::Path& filepath)
{

  PkgAdmDir admdir;
  std::vector<std::string> dirlist ;


  admdir.apply([&dirlist](const std::string& d,const std::string&& f) -> bool{
    dirlist.push_back(d+"/"+f);
    return true;
  });


#pragma omp parallel
{
   #pragma omp sections
   {
     for(std::string f: dirlist)
       {
       #pragma omp task
         {
           process(std::move(f), filepath);
         }
       }
   }
}

  return true;
}

}} // ns lookup
