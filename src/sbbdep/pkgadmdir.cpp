/*
 --------------Copyright (c) 2012-2013 H a r a l d  A c h i t z---------------
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

#include <sbbdep/pkgadmdir.hpp>

namespace sbbdep {

PkgAdmDir::PkgAdmDir(std::string path , IgnorFilterCall filter)
:m_dir(path) , m_ignoreFilter(filter)
{
  m_dir.Open();
}
//--------------------------------------------------------------------------------------------------

PkgAdmDir::~PkgAdmDir()
{
  if(m_dir.isOpen())
    m_dir.Close();
}
//--------------------------------------------------------------------------------------------------
void
PkgAdmDir::apply(DirContentCall call)
{
  std::string fname;
  while( m_dir.getNext(fname) )
    {
      if( m_ignoreFilter(fname) )
        continue;
      // TODO , change to move(fname)
      if( !call(m_dir.getDirName(), std::move(fname)) )
        break;
    }
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} /* namespace sbbdep */
