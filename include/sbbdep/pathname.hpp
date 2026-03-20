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

#ifndef SBBDEP_PATHNAME_HPP_
#define SBBDEP_PATHNAME_HPP_

#include <string>
#include <vector>

namespace sbbdep
{

  class PathName
  {
  public:
    PathName () noexcept;
    PathName (std::string url) noexcept;

    PathName (const PathName&)            = default;
    PathName& operator= (const PathName&) = default;

    PathName (PathName&&) noexcept   = default;
    PathName& operator= (PathName&&) = default;

    virtual ~PathName () noexcept = default;

    const std::string&
    str () const
    {
      return _url;
    }

    std::string base () const;
    std::string dir () const;

    bool
    empty () const
    {
      return _url.empty ();
    }

    operator const char*() const { return _url.c_str (); }
    operator const std::string&() const { return _url; }

    bool
    operator== (const PathName& other) const
    {
      return _url == other._url;
    }

    bool
    operator== (const std::string& other) const
    {
      return _url == other;
    }

    std::vector<std::string> split () const;

  protected:
    // for encapsulation in Path,..
    virtual void
    setURL (const std::string& url)
    {
      _url = url;
    }

  private:
    std::string _url;
  };

} // ns

#endif /* ...PATHNAME_HPP_ */
