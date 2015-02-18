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


#include <sbbdep/dircontent.hpp>

#include <sbbdep/error.hpp>
#include <sbbdep/log.hpp>      // for assert
#include <sbbdep/pathname.hpp> // for assert

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>


#include <memory>


namespace sbbdep {


namespace
{
  struct close_dir
  {
    void operator()(DIR* d)  const noexcept {closedir(d);}
  };
  using dir_ptr = std::unique_ptr<DIR, close_dir> ;
}//-----------------------------------------------------------------------------
//------------------------------------------------------------------------------


Dir::Dir(std::string name )
:_name(name)
{

}
//------------------------------------------------------------------------------

const std::string&
Dir::getName() const
{
  return _name ;
}
//------------------------------------------------------------------------------

bool
Dir::defaultFilter(const std::string& f)
{
  // hidden and dirs, hidden should not exist in var/adm/package
  if(f[0] == '.' ) return true;
  if(*(f.rbegin()) == '~') return true; // backuo files,
  if(f[0] == '#' && *(f.rbegin()) == '#') return true; // emacs backup files

  return false;
}
//------------------------------------------------------------------------------



std::vector<std::string>
Dir::getContent(IgnorFilter filter) const
{

  std::vector<std::string> retval;

  dir_ptr dirstm { opendir(_name.c_str()) };

  if(dirstm == nullptr)
    {
      throw ErrGeneric ("open directory " + _name );
    }


  dirent entry;
  auto result = &entry;

  for(;;)
    {
      if(readdir_r (dirstm.get(), &entry, &result) > 0)
        {
          throw ErrGeneric ("readdir_r: bad return value");
          break;
        }
      if(result != nullptr)
        {
          if(not filter (entry.d_name))
            {
              retval.emplace_back(entry.d_name);
            }
        }
      else
        {
          break;
        }
    }

  return retval ;

}
//------------------------------------------------------------------------------

void
Dir::forEach(ContentCall cb, IgnorFilter filter ) const
{
  dir_ptr dirstm { opendir (_name.c_str ()) };

  if (dirstm == nullptr)
    throw ErrGeneric ("open directory " + _name);

  dirent entry;
  auto result = &entry;

  for (;;)
    {
      if (readdir_r (dirstm.get (), &entry, &result) > 0)
        {
          auto eno = std::to_string (errno);
          throw ErrGeneric ("readdir_r: bad return value, errno " + eno);
          break;
        }
      if (result != nullptr)
        {
          if (not filter (entry.d_name))
            {
              if (not cb (_name, entry.d_name))
                {
                  break;
                }
            }
        }
      else
        {
          break;
        }
    }

}
//------------------------------------------------------------------------------


std::string PkgAdmDir::name{};

Dir
PkgAdmDir::get()
{
  SBBASSERT (not name.empty ()) ;
  return Dir{name} ;
}
//------------------------------------------------------------------------------

void PkgAdmDir::set(const std::string& n)
{
  SBBASSERT (name.empty ()) ;

  name = n ;

}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}



