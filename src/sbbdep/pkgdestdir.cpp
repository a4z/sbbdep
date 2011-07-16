/*
--------------Copyright (c) 2010-2011 H a r a l d  A c h i t z---------------
-----------------< a g e dot a 4 z at g m a i l dot c o m >------------------
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


#include "sbbdep/pkgdestdir.hpp"

#include "sbbdep/error.hpp"
#include "sbbdep/dircontent.hpp"
#include "sbbdep/dynlinked.hpp"
#include "sbbdep/path.hpp"
#include "sbbdep/log.hpp"

#include "a4z/error.hpp"

#include <iostream>

namespace sbbdep {

PkgDestDir::PkgDestDir( const PathName& pname ) :
  Pkg(pname)
{
  
}
//--------------------------------------------------------------------------------------------------

PkgDestDir::~PkgDestDir()
{
  
}
//--------------------------------------------------------------------------------------------------

bool
PkgDestDir::doLoad()
{
  
  StringList dirs;
  
  dirs.insert(dirs.end(), m_bindirs.begin(), m_bindirs.end());
  dirs.insert(dirs.begin(), m_libdirs.begin(), m_libdirs.end());
  
  DynLinked dynlinked;
  
  for (StringList::iterator pos = dirs.begin(); pos != dirs.end(); ++pos)
    {
      Path path(m_pathname.getURL() + "/" + *pos);
      path.makeRealPath(); // remove // .. dir//usr/ ..
      if (path.isFolder()) CheckDir(path.getURL(), dynlinked);
    }
  
  return true;
}
//--------------------------------------------------------------------------------------------------

void
PkgDestDir::CheckDir( const std::string& dname, DynLinked& dynlinked )
{
  
  DirContent dcont(dname);
  
  try
    {
      dcont.Open();
    }
  catch ( const Error& e )
    {
      throw; //TODO
    }

  std::string contend;
  while (dcont.getNext(contend))
    {
      if (contend == "." || contend == "..") continue;

      Path path(dcont.getDirName() + "/" + contend);
      
      if (path.isFolder())
        {
          CheckDir(path.getURL(), dynlinked);
        }
      else if (path.isRegularFile())
        {
          if (dynlinked.Open(path.getURL()))
            {
              m_dlinfos.push_back(DynLinkedInfo());
              if (!dynlinked.getInfos(m_dlinfos.back())) m_dlinfos.pop_back();
            }
        }
    }
  
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} // ns
