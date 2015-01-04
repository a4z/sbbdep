/*
 --------------Copyright (c) 2013-2015 H a r a l d  A c h i t z---------------
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

#include "featurex.hpp"

#include <sbbdep/log.hpp>

#include <iterator>
#include <string>
#include <regex>

/* TODO move this to a4sqlt3
std::ostream&
operator<<(std::ostream& stm, const a4sqlt3::DbValue& v)
{

  using namespace a4sqlt3 ;
  switch( v.getStorageType() )
    {

    case DbValueType::Null:
      stm << "<NULL>" ;
      break ;

    case DbValueType::Int64:
      stm << v.getInt64() ;
      break ;

    case DbValueType::Real:
      stm << v.getDouble() ;
      break ;

    case DbValueType::Text:
      stm << v.getString() ;
      break ;

    case DbValueType::Blob:
      stm << "<BLOB>" ;
      break ;
    default:
      stm << "unknown storage type !!" ;
      break ;
    }

  return stm ;
}
*/

namespace sbbdep
{
namespace cli
{




void
runFx(const std::string& args)
{

  LogInfo() << " internal test function " << __PRETTY_FUNCTION__  ;

#ifndef DEBUG
  LogInfo() << "not enabled in non debug builds\n" ;
  return ;
#endif

  if (!args.empty())
	LogInfo() << ", args: " << args  ;

  LogInfo() <<  "\n" ;



}

}
} // ns
