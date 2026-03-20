/*
--------------Copyright (c) 2010-2026 H a r a l d  A c h i t z---------------
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

#include <sstream>
#include <utility>

#include <iostream>

namespace sbbdep
{

  PkgName::BuildTag::BuildTag (const std::string& buildtag)
  : _num (0)
  , _tag ()
  {
    static const std::string nums      = "0123456789";
    std::string::size_type   splittpos = buildtag.find_first_not_of (nums);

    if (splittpos == std::string::npos)
      {
        std::istringstream istm (buildtag);
        istm >> _num;
      }
    else
      {
        std::istringstream istm (buildtag.substr (0, splittpos));
        istm >> _num;
        _tag = buildtag.substr (splittpos);
      }
  }
  //------------------------------------------------------------------------------
  //------------------------------------------------------------------------------

  //------------------------------------------------------------------------------

  void
  PkgName::makeDetails ()
  {
    using namespace std;
    pair<string::size_type, string::size_type> range (
        _fullname.rfind ('-', string::npos), string::npos);

    _buildstr = _fullname.substr (range.first + 1, range.second);

    _build = BuildTag (_buildstr);

    range.second = range.first - 1;
    range.first  = _fullname.rfind ('-', range.second);
    _arch = _fullname.substr (range.first + 1, range.second - range.first);

    range.second = range.first - 1;
    range.first  = _fullname.rfind ('-', range.second);
    _version = _fullname.substr (range.first + 1, range.second - range.first);

    _name = _fullname.substr (0, range.first);
  }
  //------------------------------------------------------------------------------

  std::ostream&
  operator<< (std::ostream& os, const PkgName& pkg)
  {
    os << pkg.name ();
    return os;
  }

} // ns
