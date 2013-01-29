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


#ifndef SBBDEP_DYNLINKED_HPP_
#define SBBDEP_DYNLINKED_HPP_

#include <memory>
#include <string>
#include <sbbdep/dynlinkedinfo.hpp>


//class Elf;

namespace ELFIO{
 class elfio;
}

namespace sbbdep {


class DynLinked
{
  
public:
  
  DynLinked();
  ~DynLinked();
  
  enum Arch { ArchNA = 0, Arch32 = 32 , Arch64 = 64 };
  enum Type { TypeNA= 0 , Other,  Binary , Library};
  

  bool Open(const std::string& filename);
  void Close(); 
  bool isOpen() {return m_elfreader.get() != nullptr ; }
  
  const std::string& getFileName() const { return m_filename; }
  Arch getArch() const {return m_arch;}
  Type getType()  const{ return m_type; }  
  
  bool getInfos( DynLinkedInfo& info ) const ;
  
  
  const std::string& getErrMsg()const{return m_errmsg; }
  
private:
  DynLinked(const DynLinked&);
  DynLinked& operator=(const DynLinked&);
  

  std::unique_ptr<ELFIO::elfio> m_elfreader;
  std::string m_filename;
  Arch m_arch;
  Type m_type;
  

  std::string m_errmsg; // libelf error, if used.. // TODO, check if required since elfio
 
  
};

}

#endif /* SBBDEP_DYNLINKED_HPP_ */
