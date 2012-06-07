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


#include "sbbdep/dynlinked.hpp"
#include "sbbdep/log.hpp"

//#include <iostream>

// with elfuitls instead of lib elf this may fail on other dists,
// would be.. #include <gelf.h> make this via make, 
#include <libelf/gelf.h>
#include <libelf/libelf.h>

#include <fcntl.h>

#include "sbbdep/path.hpp"
#include <boost/algorithm/string/replace.hpp>

#include "a4z/error.hpp"

namespace sbbdep {

namespace {


// leaf this here for later use...
struct CheckRRunPath
{
  const std::string& m_from_file ; 
  Path m_home; 
  
  CheckRRunPath(const std::string from_file)
  :m_from_file(from_file) , m_home( Path(from_file).getDir() )
  {
    
  }
  
  std::string operator()(const std::string& rrunpath )
    {
      LogChannelType lc = LogDebug();
      
      using boost::algorithm::replace_first_copy; 
      std::string rpreal=replace_first_copy(rrunpath,"$ORIGIN/..",m_home.getDir()) ;
      rpreal=replace_first_copy(rpreal ,"$ORIGIN", m_home.getURL()) ;
  
      
      Path pathreal = rpreal;
      if (!pathreal.isValid() || !pathreal.isFolder())
        {
          lc << "rpath does not exist:\n"
             << " from file: " << m_from_file <<"\n"  
             <<  rrunpath << " => " << pathreal <<"\n" ;
          
          return  "" ;
        }
      else 
        {
          if (pathreal.isLink())
            {
              if (!pathreal.makeRealPath()) return "";
            }
        }
      
        return pathreal; 
      }
}
;
}


DynLinked::DynLinked() :
  m_elf(0), m_filename(), m_arch(ArchNA), m_type(TypeNA)
{
  
  if (elf_version(EV_CURRENT) == EV_NONE)
    {
      std::string elferrmsg = elf_errmsg(-1);
      throw a4z::ErrorNeverReach("set elf version failed: " + elferrmsg);
      // hopefully realy a never reach, but its sensless to continue if this does not work..
    }
  
}
//--------------------------------------------------------------------------------------------------


//DynLinked::~DynLinked()
//{
//  done in hdr to see on first view that close is called auto  
//}
//--------------------------------------------------------------------------------------------------


bool
DynLinked::Open( const std::string& filename )
{
  //reset all values
  Close();
  
  m_filename = filename;
  
  int fd = 0;
  if ((fd = open(filename.c_str(), O_RDONLY, 0)) < 0)
    {
      m_errmsg = "can't open " + filename;
      return false;
    }

  m_elf = elf_begin(fd, ELF_C_READ, NULL);
  close(fd);
  
  if (!m_elf)
    {
      m_errmsg = std::string("elf_begin failed: ") + elf_errmsg(-1);
      return false;
    }

  //Elf_Kind ek = elf_kind(m_elf);
  //if (ek != ELF_K_ELF)
  if (elf_kind(m_elf) != ELF_K_ELF)
    {
      m_type=Other;
      return false; 
    }

  int elfclass = gelf_getclass(m_elf);
  if (elfclass == ELFCLASSNONE) throw a4z::ErrorNeverReach("should already have returned false");
  else if (elfclass == ELFCLASS32) m_arch = Arch32;
  else if (elfclass == ELFCLASS64) m_arch = Arch64;
  else throw a4z::ErrorNeverReach("unknown arch should not happen");
  
  GElf_Ehdr ehdr;
  if (gelf_getehdr(m_elf, &ehdr) == NULL)
    {
      m_errmsg = std::string("gelf_getclass failed: ") + elf_errmsg(-1);
      return false;
    }

  // TODO, whant to know what happens if m_type is Archive or Coff, test this..

  if (ehdr.e_type == ET_NONE) return false;
  if (ehdr.e_type == ET_EXEC) m_type = Binary;
  else if (ehdr.e_type == ET_DYN) m_type = Library;
  else if (ehdr.e_type == ET_REL) m_type=Other;
  else throw a4z::ErrorNeverReach("unknown type should not happen");
   

  return m_type==Binary || m_type == Library;
  
}
//--------------------------------------------------------------------------------------------------

void
DynLinked::Close()
{
  if (m_elf) {
      elf_end(m_elf);
      m_elf = 0;
  }
  m_arch = ArchNA;
  m_type = TypeNA;
  m_filename.clear();
  m_errmsg.clear();
}
//--------------------------------------------------------------------------------------------------

bool
DynLinked::getInfos( DynLinkedInfo& info )
{
  
  if (!(m_type == Binary || m_type == Library)) return false;

  info.filename = m_filename; 
  info.arch = m_arch; 
  
  CheckRRunPath runrpathcheck (m_filename);

  
  Elf_Scn* scn = 0;
  GElf_Shdr shdr;
  while ((scn = elf_nextscn(m_elf, scn)))
    {
      if (gelf_getshdr(scn, &shdr) != &shdr)
        {
          m_errmsg = std::string("gelf_getshdr failed: ") + elf_errmsg(-1);
          return false;
        }
      
      if (shdr.sh_type == SHT_DYNAMIC)
        {
          GElf_Dyn sym;
          Elf_Data* data = elf_getdata(scn, NULL);
          for (size_t i = 0; i < shdr.sh_size / shdr.sh_entsize; ++i)
            {
              if (!gelf_getdyn(data, i, &sym)) return false;
              if (sym.d_tag == DT_NEEDED)
                {
                  char* val = elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                  info.Needed.push_back(std::string(val));
                }
              else if (sym.d_tag == DT_SONAME)
                {
                  char* val = elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                  info.soName = val;
                }
              else if (sym.d_tag == DT_RPATH )
                { //lot stuff uses rpath or runpath, to have this info available, lets grep it here 
                  //char* val = elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                  std::string pathes=elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                 
                  for ( std::size_t spos = 0, epos = pathes.find(":", spos);
                   spos!=epos && spos!=pathes.size(); epos = pathes.find(":", spos))
                    {
                      std::string rpath = pathes.substr(spos, epos - spos) ;
                      info.RunRPaths.push_back(  rpath ) ;
                      spos = epos == std::string::npos ? std::string::npos : epos + 1;      
                    }                  
                  
                }
              else if (sym.d_tag ==  DT_RUNPATH)
                { // assume that runpath is always after rpath
                  // man ld  6.  For a native ELF linker, 
                  // the directories in "DT_RUNPATH" or "DT_RPATH" 
                  // of a shared library are searched for shared libraries needed by it. 
                  // The "DT_RPATH" entries are ignored if "DT_RUNPATH" entries exist.  
                  info.RunRPaths.clear();
                  
                  //char* val = elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                  std::string pathes=elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                  
                  for ( std::size_t spos = 0, epos = pathes.find(":", spos);
                   spos!=epos && spos!=pathes.size(); epos = pathes.find(":", spos))
                    {
                      std::string rpath = pathes.substr(spos, epos - spos) ;
                      info.RunRPaths.push_back( rpath ) ;
                      spos = epos == std::string::npos ? std::string::npos : epos + 1;      
                    }                                    
                  
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
