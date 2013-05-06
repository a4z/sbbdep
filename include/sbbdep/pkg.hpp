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


#ifndef SBBDEP_PKG_HPP_
#define SBBDEP_PKG_HPP_


#include <sbbdep/path.hpp>
#include <sbbdep/stringset.hpp>
#include <sbbdep/elffile.hpp>

#include <vector>

namespace sbbdep {



enum class PkgType{
  Unknown = 0 ,
  Installed , // var/adm/packages/.. file
  BinLib , // on file
  DestDir ,  // make install DESTDIR=$tmp/usr ... as used in build scripts to create dep files
  Archive // for future
};



class Pkg
{

  
public:  

  typedef std::vector<ElfFile> DynLinkedFiles; // TODO rename this into something better

  // TODO, merge this into something like SpecialDirs together with lddir
  static const StringSet& usualBinDirs(); // need review, see comment in cpp
  static const StringSet& usualLibDirs(); // need review, see comment in cpp

  
  static Pkg create(Path pn);


//  Pkg( const Pkg& other ) = default;
//  Pkg& operator=( const Pkg& other )= default;
//  Pkg( Pkg&& other ) = default;
//  Pkg& operator=( Pkg&& other )= default;
  ~Pkg() = default;
  
  const Path& getPath() const { return  m_path; }
  
  // if ever required to have a pkg just with file info, split file and dynlink loading
  bool Load() ;
  bool isLoaded() const { return m_floaded ; }

  const DynLinkedFiles& getDynLinked() const { return m_dlfiles; }
  
  PkgType getType() const {return m_type; }
  
  ElfFile::Arch getArch() const;

protected:

  //will see if the factory method will go into this class...
  Pkg( const Path&& pname , PkgType type);

  
  //needs to be special for each pkg type
  bool doLoadOneBinLib() ;
  bool doLoadDestDir() ;
  bool doLoadInstalled() ;

  const Path m_path; // name  of
  PkgType m_type ;
  bool m_floaded ; // files loaded...

  
  DynLinkedFiles m_dlfiles;
  
  
};



}

#endif /* PKG_HPP_ */
