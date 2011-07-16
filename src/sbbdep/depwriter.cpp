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


#include "sbbdep/depwriter.hpp"

#include "sbbdep/pkg.hpp"
#include "sbbdep/pkgarchiv.hpp"
#include "sbbdep/pkgdestdir.hpp"
#include "sbbdep/pkgfile.hpp"
#include "sbbdep/pkgonebinlib.hpp"

#include <iostream>
#include <string>


namespace sbbdep {

DepWriter::DepWriter()
{
}
DepWriter::~DepWriter()
{
}



//--------------------------------------------------------------------------------------------------
void
DepWriteList::WriteOut( Pkg& pkg, std::ostream& ostm )
{
  std::string pkgtypename ; 
  
  // todo,  think about add the name into pkg, this code is ugly
  if ( dynamic_cast<PkgArchiv*>(&pkg) ) pkgtypename = "Archiv" ;
  else if ( dynamic_cast<PkgDestDir*>(&pkg) ) pkgtypename = "Dir" ;
  else if ( dynamic_cast<PkgFile*>(&pkg) ) pkgtypename = "Package" ;
  else if ( dynamic_cast<PkgOneBinLib*>(&pkg) ) pkgtypename = "File" ;
  else pkgtypename="Unknown Package Type" ;
  
  
  
}
//--------------------------------------------------------------------------------------------------



} // ns
