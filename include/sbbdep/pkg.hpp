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


#ifndef SBBDEP_PKG_HPP_
#define SBBDEP_PKG_HPP_


#include "sbbdep/pathname.hpp"
#include "sbbdep/stringset.hpp"
#include "sbbdep/dynlinkedinfolist.hpp"

namespace sbbdep {


class Pkg
{
 
  
public:  

  
  Pkg( const PathName& pname );

  virtual ~Pkg() = 0;
  
  const PathName& getPathName() const { return  m_pathname; }  
  
  // if ever required to have a pkg just with file infos, split file and dynlink loading
  bool Load() { 
    m_floaded = doLoad() ;  
    return m_floaded ;
  }
  
  bool isLoaded() const { return m_floaded ; }

  const DynLinkedInfoList& getDynLinkedInfos() const { return m_dlinfos; } 
  
  void doTestPrint();  
  
protected:
  Pkg( const Pkg& other );
  Pkg& operator=( const Pkg& other );
  
  //needs to be spezial for each pkg type
  virtual bool doLoad() = 0;
 
  PathName m_pathname; // name  of 
  bool m_floaded ; // files loaded...

  StringSet m_libdirs; //dirs where libs are ( /lib(64) /usr/lib(64) .. ), used for search 
  StringSet m_bindirs; //dirs where other dynamicly linked are ( /(s)bin) /usr/(s)bin /usr/libexec ), used for search  
  
  DynLinkedInfoList m_dlinfos;
  
  
};



}

#endif /* PKG_HPP_ */
