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


#include <sbbdep/elffile.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/error.hpp>

#include <elfio/elfio.hpp>


namespace sbbdep {


ElfFile::ElfFile(const PathName& name) noexcept
  : m_name(name)
  , m_arch(ArchNA)
  , m_type(TypeNA)
  , m_soName()
  , m_needed()
  , m_rrunpaths()
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
//--------------------------------------------------------------------------------------------------


ElfFile::~ElfFile()
{

}
//--------------------------------------------------------------------------------------------------

void
ElfFile::load()
{
  // firewall against invalid pathnames
  if(not Path(m_name).isRegularFile())
    return ;

  ELFIO::elfio elfreader;

  if ( not elfreader.load( m_name ) ) {
      return;
  }

  int elfclass = elfreader.get_class();
  if (elfclass == ELFCLASSNONE)
    throw ErrUnexpected("should already have returned false");
  else if (elfclass == ELFCLASS32)
    m_arch = Arch32;
  else if (elfclass == ELFCLASS64)
    m_arch = Arch64;
  else
    throw ErrUnexpected("unknown arch should not happen");


  ELFIO::Elf_Half type = elfreader.get_type();

  if (type == ET_NONE)
    m_type = TypeNA;
  else if (type == ET_EXEC)
    m_type = Binary;
  else if (type == ET_DYN)
    m_type = Library;
  else
    m_type=Other;


  if( !( m_type == Binary || m_type == Library ) )
    return ;



  ELFIO::Elf_Half n = elfreader.sections.size();
  for(ELFIO::Elf_Half i = 0; i < n; ++i)
    { // For all sections
      ELFIO::section* sec = elfreader.sections[i];
      if( SHT_DYNAMIC == sec->get_type() )
        {
          ELFIO::dynamic_section_accessor dynamic(elfreader, sec);

          if( not dynamic.get_entries_num() > 0 )
            return ;

          for(ELFIO::Elf_Xword i = 0; i < dynamic.get_entries_num(); ++i)
            {
              ELFIO::Elf_Xword tag = DT_NULL;
              ELFIO::Elf_Xword value;
              std::string val;
              dynamic.get_entry(i, tag, value, val);

              if( tag == DT_NEEDED )
                {
                  m_needed.push_back(std::string(val));
                }
              else if( tag == DT_SONAME )
                {
                  m_soName = val;
                }
              else if( tag == DT_RPATH )
                {
                  std::string pathes = val;

                  for(std::size_t spos = 0, epos = pathes.find(":", spos);
                      spos != epos && spos != pathes.size(); epos = pathes.find(":", spos))
                    {
                      std::string rpath = pathes.substr(spos, epos - spos);
                      m_rrunpaths.push_back(rpath);
                      spos = epos == std::string::npos ? std::string::npos : epos + 1;
                    }

                }
              else if( tag == DT_RUNPATH )
                { // assume that runpath is always after rpath
                  m_rrunpaths.clear();

                  std::string pathes = val;

                  for(std::size_t spos = 0, epos = pathes.find(":", spos);
                      spos != epos && spos != pathes.size(); epos = pathes.find(":", spos))
                    {
                      std::string rpath = pathes.substr(spos, epos - spos);
                      m_rrunpaths.push_back(rpath);
                      spos = epos == std::string::npos ? std::string::npos : epos + 1;
                    }

                }

              else if( DT_NULL == tag )
                {
                  break;
                }
            }

          return ;

        }
    }


}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


bool isElfBinOrElfLib(const PathName& pn)
{
  ELFIO::elfio elfreader;

  if ( not elfreader.load( pn ) ) {
      return false;
  }

  bool retval = false;
  int elfclass = elfreader.get_class();
  if( elfclass == ELFCLASS32 || elfclass == ELFCLASS64 )
    {
      ELFIO::Elf_Half type = elfreader.get_type();
      if(type == ET_EXEC || type == ET_DYN)
        retval = true;
    }

  return retval;
}


bool isElfLib(const PathName& pn)
{
  ELFIO::elfio elfreader;

  if ( not elfreader.load( pn ) ) {
      return false;
  }

  bool retval = false;
  int elfclass = elfreader.get_class();
  if( elfclass == ELFCLASS32 || elfclass == ELFCLASS64 )
    {
      ELFIO::Elf_Half type = elfreader.get_type();
      if (type == ET_DYN)
        retval = true;
    }

  return retval;
}



//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
