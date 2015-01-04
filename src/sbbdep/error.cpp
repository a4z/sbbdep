/*
--------------Copyright (c) 2009-2015 H a r a l d  A c h i t z---------------
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

#include <sbbdep/error.hpp>

#include <ostream>
#include <map>

namespace sbbdep{


namespace {
  std::string ErrCodeName(ErrCode ec)
  {
    static const std::map<ErrCode, std::string> names{
      { ErrCode::TODO               , "TODO" } ,
      { ErrCode::UNEXPECTED         , "UNEXPECTED" }
    };

    auto i = names.find( ec );
    if( i != std::end( names ) )
      return i->second;

    return std::to_string( (int)ec );
  }
}//------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

Error::Error( std::string info )
:_info(std::move(info))
{
}
//--------------------------------------------------------------------------------------------------
Error::~Error() noexcept
{
}
//--------------------------------------------------------------------------------------------------

const std::string&
Error::info() const
{
  return _info ;
}
//--------------------------------------------------------------------------------------------------

void
Error::toStream(std::ostream& os) const
{
  os << "sbbdep::" << ErrCodeName(id()) << ":" << info();
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------

std::ostream& operator<< (std::ostream& os, const Error& e)
{
  e.toStream(os) ;
  return os;
}
//--------------------------------------------------------------------------------------------------


}
