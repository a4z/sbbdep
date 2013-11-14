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


#ifndef SBBDEP_ELFFILE_HPP_
#define SBBDEP_ELFFILE_HPP_


#include <string>
#include <vector>
#include <sbbdep/pathname.hpp>

namespace sbbdep {

// opens the file and reads the info, if file is n
class ElfFile
{
  
public:
  typedef std::vector<std::string> StringVec;
  enum Arch { ArchNA = 0, Arch32 = 32 , Arch64 = 64 };
  enum Type { TypeNA= 0 , Other,  Binary , Library};

  // calls load, catches all exceptions and swallows them
  // so if arch and type stays NA than either not an elf file or file does not exist
  ElfFile(const PathName& name) noexcept ;
  
  ~ElfFile();

  const PathName& getName() const { return _name; }
  
  Arch getArch() const {return _arch;}

  Type getType()  const{ return _type; }  
  
  std::string soName() const {
    if(getType()== ElfFile::Library && _soName.empty())
      return getName().getBase();

    return _soName ;
  }
  
  const StringVec& getNeeded() const { return _needed ; }
  
  const StringVec& getRRunPaths() const { return _rrunpaths ; }
  
  bool isBinaryOrLibrary(){ return getType() == ElfFile::Binary || getType() == ElfFile::Library ;}

  bool hasRPath() const {return !_hasRunPath && !_rrunpaths.empty();}
  bool hasRunPath() const {return _hasRunPath && !_rrunpaths.empty();}

private:
  

  PathName _name;
  Arch _arch ;
  Type _type ;
  
  std::string _soName;
  StringVec _needed;
  StringVec _rrunpaths;

  bool _hasRunPath;

  void load();
 
  
};

// if just the info is required if given path points to a bin or lib file, this is ok
bool isElfBinOrElfLib(const PathName& pn);
bool isElfLib(const PathName& pn);


}

#endif /* SBBDEP_DYNLINKED_HPP_ */
