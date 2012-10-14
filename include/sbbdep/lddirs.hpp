/*
--------------Copyright (c) 2011-2012 H a r a l d  A c h i t z---------------
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


#ifndef SBBDEP_LDDIRS_HPP_
#define SBBDEP_LDDIRS_HPP_

#include <sbbdep/stringset.hpp>

namespace sbbdep
{
// get dir names from /etc/ld.so.conf
class LDDirs
{
  
public:
  LDDirs();
  ~LDDirs();
  
  const StringSet& getLdDirs() const { return m_lddirs;} 
  const StringSet& getLdLnkDirs() const { return m_ldlnkdirs;}
  
  const StringSet& readLdDirs();
  const StringSet& readLdLinkDirs();
  
private:
  StringSet m_lddirs;
  StringSet m_ldlnkdirs;
};


}


#endif /* LDDIRS_HPP_ */



