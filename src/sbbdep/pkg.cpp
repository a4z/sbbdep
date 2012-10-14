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



#include <sbbdep/pkg.hpp>

namespace sbbdep {

Pkg::Pkg( const PathName& pname )
: m_pathname ( pname ) 
, m_floaded(false)
{

  // this seems to be ok on slackware
  // if others exist, add them to list
  // TODO, make this configurabel through store those in the db ?
  m_bindirs.insert( "/sbin" ) ;
  m_bindirs.insert( "/usr/sbin" ) ;
  m_bindirs.insert( "/bin" ) ;
  m_bindirs.insert( "/usr/bin" ) ;
  m_bindirs.insert( "/usr/libexec" ) ;

  m_libdirs.insert( "/lib" ) ;
  m_libdirs.insert( "/usr/lib" ) ;
  m_libdirs.insert( "/lib64" ) ;
  m_libdirs.insert( "/usr/lib64" ) ;  

}
//--------------------------------------------------------------------------------------------------

Pkg::~Pkg()
{
  
}
//--------------------------------------------------------------------------------------------------

void
Pkg::doTestPrint()
{
  
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} // ns
