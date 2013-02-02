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


#include "lookup.hpp"

#include <sbbdep/pkg.hpp>


#include <iostream>
#include <string>


namespace sbbdep {
namespace lookup{

// helper space
namespace {

bool isPublicLib(const std::string dirname, const std::string soname)
{
  return true;
}

}// helper space


bool who_needs(Pkg& pkg , Log::ChannelType out, bool addVersion)
{
  // check, if got a filename, if this is a ldpath stuff or public or absolut no known path..

  for(const DynLinkedInfo& dli : pkg.getDynLinkedInfos())
    {
      if(dli.soName.empty())
        continue; // nothing to do if this not a so file

      // ok, it is a so, does it exist more than once on the system
      // is it in a public findable place
      // who finds it and who may prefere an ohter one for eg within own package or via rpath..

      std::string indir = dli.filename.getDir();
      // check if this lib is public findable or more a private stuff of a package
      // this will be complecated if ldusr dir is used...
      if( isPublicLib(indir, dli.soName) )
        {

        }
      else
        {

        }
      //int arch;
      //std::string soName;
      /*
       *  ok, hab den soname und weiss wo das ding liegt.
       *  schauen wer das alles braucht,
       *  ergebniss untersuchen, braucht es wirklich das oder etwas anderes... ?
       *
      */
    }

  return false;
}


bool explain_who_needs(Pkg& pkg , Log::ChannelType out)
{
  // ok, got a file ....
  PathName dirname = pkg.getPathName().getDir();
  std::string filename = pkg.getPathName().getBase();

  // who needs the link name of this file,
  // where do I get the link name from  ? ..

  // need the rpath informations also,
  //check if this is in the rpath ,


  return false;
}



}
} // ns
