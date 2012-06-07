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



#ifndef SBBDEP_FILEMAGIC_HPP_
#define SBBDEP_FILEMAGIC_HPP_

#include <magic.h>
#include <string>
#include "a4z/single.hpp"

namespace sbbdep {

class PathName; 

  

class FileMagic  
{
  
  //friend class a4z::Single<FileMagic>;  
  
public:  

  FileMagic();
  ~FileMagic();
  
  
  std::string getMagicNone(const PathName& pathname);
  std::string getMagicMimeType(const PathName& pathname);
  std::string getMagicMimeEncoding(const PathName& pathname);
  std::string getMagicNoCheckElf(const PathName& pathname);
  
  //until i know about threadsafty, provide both...
  std::string getMagicNoneLocked(const PathName& pathname);
  std::string getMagicMimeTypeLocked(const PathName& pathname);
  std::string getMagicMimeEncodingLocked(const PathName& pathname);
  std::string getMagicNoCheckElfLocked(const PathName& pathname);  
  
  
private:
  magic_t m_cookie; 
  
  FileMagic(const FileMagic&);
  const FileMagic& operator=(const FileMagic&);
  
  
};


struct  FMagSingle : public a4z::Single<FileMagic>{}; 

}

#endif /* FILEMAGIC_HPP_ */
