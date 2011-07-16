/*
--------------Copyright (c) 2010-2011 H a r a l d  A c h i t z---------------
-----------------< a g e dot a 4 z at g m a i l dot c o m >------------------
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

#include <iostream>

// with elfuitls instead of lib elf this may fail on other dists,
// would be.. #include <gelf.h> make this via make, 
#include <libelf/gelf.h>
#include <libelf/libelf.h>

#include <fcntl.h>

#include "a4z/error.hpp"

namespace sbbdep {



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
              /* not nedded for dynlink
              else if (sym.d_tag == DT_RPATH)
                {
                  char* val = elf_strptr(m_elf, shdr.sh_link, sym.d_un.d_val);
                  info.RPath = val;
                }
              */
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
