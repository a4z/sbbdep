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


#include <sbbdep/dynlinked.hpp>
#include <sbbdep/log.hpp>

//#include <iostream>

// with elfuitls instead of lib elf this may fail on other dists,
// would be.. #include <gelf.h> make this via make, 
//#include <libelf/gelf.h>
//#include <libelf/libelf.h>

//#include <fcntl.h>

#include <elfio/elfio.hpp>

#include <sbbdep/path.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <a4z/error.hpp>

namespace sbbdep {


DynLinked::DynLinked() :
  m_elfreader(), m_filename(), m_arch(ArchNA), m_type(TypeNA)
{

}
//--------------------------------------------------------------------------------------------------


DynLinked::~DynLinked()
{
  m_elfreader.reset(nullptr);
}
//--------------------------------------------------------------------------------------------------


bool
DynLinked::Open( const std::string& filename )
{
  //reset all values
  Close();
  
  m_filename = filename;
  
  m_elfreader.reset(new ELFIO::elfio() ) ;

  if ( not m_elfreader->load( m_filename ) ) {
      return false; // TODO , not an elf
  }

  int elfclass = m_elfreader->get_class();
  if (elfclass == ELFCLASSNONE) throw a4z::ErrorNeverReach("should already have returned false");
  else if (elfclass == ELFCLASS32) m_arch = Arch32;
  else if (elfclass == ELFCLASS64) m_arch = Arch64;
  else throw a4z::ErrorNeverReach("unknown arch should not happen");


  ELFIO::Elf_Half type = m_elfreader->get_type();

  if (type == ET_NONE) m_type = TypeNA;
  else if (type == ET_EXEC) m_type = Binary;
  else if (type == ET_DYN) m_type = Library;
  else m_type=Other;

  return m_type==Binary || m_type == Library;


}
//--------------------------------------------------------------------------------------------------

void
DynLinked::Close()
{
  m_elfreader.reset(nullptr);
  m_arch = ArchNA;
  m_type = TypeNA;
  m_filename.clear();
  m_errmsg.clear();
}
//--------------------------------------------------------------------------------------------------

bool
DynLinked::getInfos(DynLinkedInfo& info) const
{

  if( not m_elfreader )
    throw a4z::ErrorTodo("reader not open");

  if( !( m_type == Binary || m_type == Library ) )
    return false;

  info.filename = m_filename;
  info.arch = m_arch;

  ELFIO::Elf_Half n = m_elfreader->sections.size();
  for(ELFIO::Elf_Half i = 0; i < n; ++i)
    { // For all sections
      ELFIO::section* sec = m_elfreader->sections[i];
      if( SHT_DYNAMIC == sec->get_type() )
        {
          ELFIO::dynamic_section_accessor dynamic(*m_elfreader, sec);

          if( not dynamic.get_entries_num() > 0 )
            return false;

          for(ELFIO::Elf_Xword i = 0; i < dynamic.get_entries_num(); ++i)
            {
              ELFIO::Elf_Xword tag;
              ELFIO::Elf_Xword value;
              std::string val;
              dynamic.get_entry(i, tag, value, val);

              if( tag == DT_NEEDED )
                {
                  info.Needed.push_back(std::string(val));
                }
              else if( tag == DT_SONAME )
                {
                  info.soName = val;
                }
              else if( tag == DT_RPATH )
                {
                  std::string pathes = val;

                  for(std::size_t spos = 0, epos = pathes.find(":", spos);
                      spos != epos && spos != pathes.size(); epos = pathes.find(":", spos))
                    {
                      std::string rpath = pathes.substr(spos, epos - spos);
                      info.RunRPaths.push_back(rpath);
                      spos = epos == std::string::npos ? std::string::npos : epos + 1;
                    }

                }
              else if( tag == DT_RUNPATH )
                { // assume that runpath is always after rpath
                  info.RunRPaths.clear();

                  std::string pathes = val;

                  for(std::size_t spos = 0, epos = pathes.find(":", spos);
                      spos != epos && spos != pathes.size(); epos = pathes.find(":", spos))
                    {
                      std::string rpath = pathes.substr(spos, epos - spos);
                      info.RunRPaths.push_back(rpath);
                      spos = epos == std::string::npos ? std::string::npos : epos + 1;
                    }

                }

              else if( DT_NULL == tag )
                {
                  break;
                }
            }

          return true;

        }
    }

  return false;

}
//--------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
