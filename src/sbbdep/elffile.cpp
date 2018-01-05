/*
--------------Copyright (c) 2010-2016 H a r a l d  A c h i t z---------------
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


#include <sbbdep/elffile.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/error.hpp>

#include <elfio/elfio.hpp>

#include <boost/algorithm/string/replace.hpp>


namespace sbbdep {


ElfFile::ElfFile() noexcept
  : _name{}
  , _arch{ArchNA}
  , _type{TypeNA}
  , _soName{}
  , _needed{}
  , _rrunpaths{}
  , _hasRunPath{false}
{

}
//------------------------------------------------------------------------------

ElfFile::ElfFile(Path name) noexcept
  : _name{std::move(name)}
  , _arch{ArchNA}
  , _type{TypeNA}
  , _soName{}
  , _needed{}
  , _rrunpaths{}
  , _hasRunPath{false}
{
  try
  {
      load();
  }
  catch(...)
  {
      ;
  }

}
//------------------------------------------------------------------------------


ElfFile::~ElfFile()
{

}
//------------------------------------------------------------------------------


void
ElfFile::load()
{

  ELFIO::elfio elfreader;

  if (not elfreader.load (_name))
    {
      return;
    }

  int elfclass = elfreader.get_class ();
  if (elfclass == ELFCLASSNONE)
    throw ErrUnexpected ("should already have returned false");
  else if (elfclass == ELFCLASS32)
    _arch = Arch32;
  else if (elfclass == ELFCLASS64)
    _arch = Arch64;
  else
    throw ErrUnexpected ("unknown arch should not happen");

  ELFIO::Elf_Half type = elfreader.get_type ();

  if (type == ET_NONE)
    _type = TypeNA;
  else if (type == ET_EXEC)
    _type = Binary;
  else if (type == ET_DYN)
    _type = Library;
  else
    _type = Other;

  if (!(_type == Binary || _type == Library))
    return;

  ELFIO::Elf_Half n = elfreader.sections.size ();
  for (ELFIO::Elf_Half i = 0; i < n; ++i)
    { // For all sections
      ELFIO::section* sec = elfreader.sections [i];
      if ( SHT_DYNAMIC == sec->get_type ())
        {
          ELFIO::dynamic_section_accessor dynamic (elfreader, sec);

          if (not (dynamic.get_entries_num () > 0))
            return;

          for (ELFIO::Elf_Xword i = 0; i < dynamic.get_entries_num (); ++i)
            {
              ELFIO::Elf_Xword tag = DT_NULL;
              ELFIO::Elf_Xword value;
              std::string val;
              dynamic.get_entry (i, tag, value, val);

              if (tag == DT_NEEDED)
                {
                  _needed.push_back (std::string (val));
                }
              else if (tag == DT_SONAME)
                {
                  _soName = val;
                }
              else if (tag == DT_RPATH)
                { //LogDebug () << _name.str() << " uses DT_RPATH" ;
                  std::string pathes = val;

                  for (std::size_t spos = 0, epos = pathes.find (":", spos);
                      spos != epos && spos != pathes.size (); epos =
                          pathes.find (":", spos))
                    {
                      std::string rpath = pathes.substr (spos, epos - spos);
                      _rrunpaths.push_back (rpath);
                      spos =
                          epos == std::string::npos ?
                              std::string::npos : epos + 1;
                    }

                }
              else if (tag == DT_RUNPATH)
                { //  rpath only if runpath does not exist
                  //  so we can overwrite ..
                  _rrunpaths.clear ();
                  _hasRunPath = true;

                  std::string pathes = val;

                  for (std::size_t spos = 0, epos = pathes.find (":", spos);
                      spos != epos && spos != pathes.size (); epos =
                          pathes.find (":", spos))
                    {
                      std::string rpath = pathes.substr (spos, epos - spos);
                      _rrunpaths.push_back (rpath);
                      spos =
                          epos == std::string::npos ?
                              std::string::npos : epos + 1;
                    }

                }

              else if ( DT_NULL == tag)
                {
                  break;
                }
            }

          // TODO , double check this!
          if (_type == Library and _soName.empty ())
            _soName = _name.base ();

          return;

        }
    }

}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

std::string
replaceORIGIN(const std::string& originstr,
                       const std::string& fromfile)
{
  sbbdep::PathName destfile(fromfile);
  using boost::algorithm::replace_first_copy;

  // there is either $ORIGIN/.. or $ORIGIN in the name
  // which we need to make absolute

  return replace_first_copy(
           replace_first_copy(originstr,"$ORIGIN/..",destfile.dir()),
           "$ORIGIN",
           destfile.str()) ;
}
//------------------------------------------------------------------------------


std::string
replaceLIB(const std::string& str, ElfFile::Arch arch)
{ // $LIB

  using boost::algorithm::replace_first_copy;

  if (arch == ElfFile::Arch32)
    {   // lib
      return replace_first_copy(str,"$LIB","lib") ;
    }
  else if (arch == ElfFile::Arch64)
    { // lib64
      return replace_first_copy(str,"$LIB","lib64") ;
    }

  throw ErrUnexpected("replaceLIB ElfFile::ArchNA") ;

}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}
