/*
--------------Copyright (c) 2010-2013 H a r a l d  A c h i t z---------------
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


#include <sbbdep/pkgname.hpp>

#include <utility>
#include <sstream>

#include <iostream>

namespace sbbdep {

PkgName::BuildTag::BuildTag(const std::string& buildtag)
: m_num(0)
, m_tag()
{
  
  static const std::string nums = "0123456789";
  std::string::size_type splittpos = buildtag.find_first_not_of(nums) ;

  
  if ( splittpos == std::string::npos )
    {
      std::istringstream istm(buildtag);
      istm >> m_num;        
    }
  else 
    {
      std::istringstream istm(buildtag.substr(0, splittpos));
      istm >> m_num;
      m_tag = buildtag.substr(splittpos);
    }
  

}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------


void 
PkgName::makeDetails()
{

  using namespace std;
  pair< string::size_type, string::size_type > 
          range(m_fullname.rfind('-', string::npos), string::npos);

  m_buildstr = m_fullname.substr(range.first + 1, range.second);

  m_build = BuildTag(m_buildstr); 
  
  range.second = range.first - 1;
  range.first = m_fullname.rfind('-', range.second);
  m_arch = m_fullname.substr(range.first + 1, range.second - range.first);
  

  range.second = range.first - 1;
  range.first = m_fullname.rfind('-', range.second);
  m_version = m_fullname.substr(range.first + 1, range.second - range.first);

  m_name = m_fullname.substr(0, range.first);
  
  
}


std::ostream& operator<<(std::ostream& os, const PkgName& pkg)
{
  os << pkg.Name();
  return os; 
}






} // ns
