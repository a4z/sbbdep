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


#include "sbbdep/pkfab.hpp"

#include "sbbdep/pkgdestdir.hpp"
#include "sbbdep/pkgfile.hpp"
#include "sbbdep/pkgonebinlib.hpp"

#include "sbbdep/path.hpp"
#include "sbbdep/pathname.hpp"
#include "sbbdep/filemagic.hpp"
#include "sbbdep/log.hpp"
#include "sbbdep/error.hpp"

#include "a4z/error.hpp"

namespace sbbdep {

PkFab::PkFab()
{
  
}
//--------------------------------------------------------------------------------------------------


PkFab::~PkFab()
{
  
  while (m_pkmap.begin() != m_pkmap.end())
    {
      Pkg* del = m_pkmap.begin()->second;
      m_pkmap.erase(m_pkmap.begin());
      delete del;
    }
  
}
//--------------------------------------------------------------------------------------------------

Pkg*
PkFab::createPkg( const PathName& pn, bool replaceLoaded )
{
  
  Path path(pn);
  if (!path.isAbsolute()) path.makeAbsolute();
  path.makeRealPath();
  
  PKMap::iterator fiter = m_pkmap.find(&path);
  if (fiter != m_pkmap.end() )
    {
      if (replaceLoaded)
        unloadPkg(path);
      else
        return fiter->second;
    }

  Pkg* retVal = 0;
  
  if (path.isFolder())
    {
      
      PkgDestDir* pkdd = new PkgDestDir(path);
      
      std::pair< PKMap::iterator, bool > inpair = m_pkmap.insert(std::make_pair(
          &(pkdd->getPathName()), pkdd));
      if (inpair.second)
        retVal = pkdd;
      else
        {
          delete pkdd;
          retVal = inpair.first->second;
        }
      
    }
  else if (path.isRegularFile())
    {
      //txt file in /var/...(somewhere)/packages ?

      // tgz txz file ?
      std::string mime = FMagSingle::get()->getMagicMimeType(path.getURL());
      if (PathName(path.getDir()).getBase() == "packages") 
        {
          if (mime != "text/plain")
            {
              LogInfo()<<"Waring: "<< path << " returns to be " << mime;
              LogInfo()<< " should be text/plain"<< std::endl; 
            }

            PkgFile* pkg = new PkgFile(path);
            addPgk(pkg) ; //don't forget, in case of problem pkg will be deleted
            retVal = pkg ;
        }
      //          tgz - Slackware package archive compressed using gzip
      //          tbz - Slackware package archive compressed using bzip2
      //          tlz - Slackware package archive compressed using lzma
      //          txz - Slackware package archive compressed using xz 
      //TODO ^^      
      else if (mime == "application/x-xz" || mime == "application/x-gzip")
        {
          //
        }
      else if (mime == "application/x-sharedlib" || mime == "application/x-executable")
        {
          PkgOneBinLib* pkg = new PkgOneBinLib(path);
          addPgk(pkg) ; //remember, in case of problem pkg will be deleted, think about if ever errorhandled
          retVal = pkg;
        }
      else //  
        {
          LogError() << " unknown package format : " + path.getURL();
          LogError() << " mime was : " + mime;

          throw ErrPkg("unknown package format : " + path.getURL()); // 
        }
      
    }
  else
    {
      throw ErrPkg("illegal url " + path.getURL() + "(orig:" + pn.getURL() + ")");
    }

  // some self protection
  if (retVal->getPathName() != path) throw a4z::ErrorNeverReach("pkg without absolut real path");
  
  return retVal;
  
}
//--------------------------------------------------------------------------------------------------

void 
PkFab::addPgk(Pkg* pkg)
{
  typedef std::pair< PKMap::iterator, bool > InsertResult ;
  InsertResult inpair = m_pkmap.insert(std::make_pair( &(pkg->getPathName()), pkg) );
  
  if (!inpair.second)
    {
      delete pkg; pkg = 0; 
      throw a4z::ErrorNeverReach("add pkg with a path that already exist");
    }
  
}
//--------------------------------------------------------------------------------------------------

bool
PkFab::havePkg( const PathName& pn )
{
  return m_pkmap.find(&pn) != m_pkmap.end();
}
//--------------------------------------------------------------------------------------------------

bool
PkFab::unloadPkg( const PathName& pn )
{
  bool retVal = false;
  PKMap::iterator fiter = m_pkmap.find(&pn);
  if (fiter != m_pkmap.end())
    {
      Pkg* del = fiter->second;
      m_pkmap.erase(fiter);
      delete del;
      retVal = true;
    }
  
  return retVal;
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
