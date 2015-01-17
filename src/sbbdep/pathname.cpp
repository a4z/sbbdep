/*
 --------------Copyright (c) 2010-2015 H a r a l d  A c h i t z---------------
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

#include <sbbdep/pathname.hpp>
#include <sbbdep/error.hpp>


#include <algorithm>
#include <climits>

#include <libgen.h>

namespace sbbdep {

PathName::PathName () noexcept
: _url()
{

}
//------------------------------------------------------------------------------

PathName::PathName (std::string url) noexcept
: _url(std::move(url))
{

}
//------------------------------------------------------------------------------

std::string
PathName::base () const
{
  SBBASSERT (_url.size () < PATH_MAX) ;

  char path [PATH_MAX];
  std::copy (_url.begin (), _url.end (), std::begin (path));
  path [_url.size ()] = '\0';
  return basename (path);
}

//------------------------------------------------------------------------------
std::string
PathName::dir () const
{
  SBBASSERT (_url.size () < PATH_MAX) ;

  char path [PATH_MAX];
  std::copy (_url.begin (), _url.end (), std::begin (path));
  path [_url.size ()] = '\0';
  return dirname (path);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}//ns

