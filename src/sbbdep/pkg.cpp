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

#include <sbbdep/pkg.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/dircontent.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/lddirs.hpp>

#include <fstream>

namespace sbbdep {



const Pkg::StringVec&
usualBinDirs()
{
  return getLDDirs().getLdDirs() ;
}
const Pkg::StringVec&
usualLibDirs()
{
  static const Pkg::StringVec data
    { "/lib", "/usr/lib", "/lib64", "/usr/lib64" };
  return data;
}

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------


// former pkg fab, try to work without fmagic, should work, ...
// will possily change in future
Pkg
Pkg::create(const Path& path, PkgType type_hint)
{

  Path pkgpath = path ;
  PkgType pkgtype= PkgType::Unknown ;

  pkgpath.makeRealPath();


  if(type_hint==PkgType::Unknown)
    {
      if( pkgpath.isFolder() )
        {
          pkgtype = PkgType::DestDir ;
        }
      else if( pkgpath.isRegularFile() )
        {
          Path admpath(PkgAdmDir.getName());
          admpath.makeRealPath();
          if( pkgpath.dir() == admpath.str() )
            {
              pkgtype = PkgType::Installed;
            }
          else if( isElfBinOrElfLib(pkgpath) )
            {
              pkgtype = PkgType::BinLib;
            }
        }
    }
  else
    {
      if(type_hint == PkgType::DestDir)
        {
          if( pkgpath.isFolder() )
            {
              pkgtype = PkgType::DestDir ;
            }

        }
      else if (type_hint == PkgType::Installed)
        {
          Path admpath(PkgAdmDir.getName());
          admpath.makeRealPath();
          if( pkgpath.dir() == admpath.str() )
            {
              pkgtype = PkgType::Installed ;
            }
         }
      else if (type_hint == PkgType::BinLib)
        {
          if( isElfBinOrElfLib(pkgpath) )
            {
              pkgtype = PkgType::Installed ;
            }
        }
    }

  // if we have nothing usefull we create an unknow package...
  return Pkg(std::move(pkgpath), pkgtype);

}
//------------------------------------------------------------------------------


Pkg::Pkg(Path pname, PkgType type)
    : _path(std::move(pname)), _type(type), _loaded(false)
{

}
//------------------------------------------------------------------------------


bool
Pkg::Load()
{
  if(isLoaded())
    {
      throw ErrGeneric("Pkg already loaded") ;
    }

  switch (_type)
    {
    case PkgType::Installed:
      _loaded =doLoadInstalled();
      break;

    case PkgType::DestDir:
      _loaded = doLoadDestDir();
      break;

    case PkgType::BinLib:
      _loaded = doLoadOneBinLib();
      break;

    default:
      break;
    };
  return _loaded;
}
//------------------------------------------------------------------------------

bool
Pkg::doLoadOneBinLib()
{

  ElfFile elfile(_path);
  if( elfile.isBinaryOrLibrary() )
    _elfFiles.push_back(elfile);


  return true;
}
//------------------------------------------------------------------------------

bool
Pkg::doLoadDestDir()
{
  // recursive go through given dir, check for dynlinked in it, if found , added to m_dlinfos
  std::function<void(const std::string&)> checkdir;
  checkdir = [this, &checkdir ]( const std::string& dname ) -> void
  {
    auto entries = Dir{dname}.getContent();

    for(auto&& entrie: entries)
      {
        Path path(dname + "/" + entrie);

        if (path.isFolder())
          {
            checkdir(path.str());
          }
        else if (path.isRegularFile())
          {
            ElfFile elfile(path);
            if( elfile.isBinaryOrLibrary() )
              _elfFiles.push_back(elfile);
          }
      }

  };

  auto dirhandler = [this, checkdir](const std::string& dirname)
    {
    Path path(this->_path.str() + "/" + dirname);
    path.makeRealPath(); // remove // .. dir//usr/ ..
    if (path.isFolder())
      checkdir(path.str());
    };

  for(auto& d : usualBinDirs())
    dirhandler(d);

  for(auto& d : usualLibDirs())
    dirhandler(d);

  return true;
}
//------------------------------------------------------------------------------

bool
Pkg::doLoadInstalled()
{
  //check if given path name is a bin/lib directory
  auto isToCheck = [this] (const PathName& pn) -> bool {
    for(auto& s : usualBinDirs()){
        if ( not pn.str().compare( 0, s.size() , s ) )
          return true;
    }
    for(auto& s : usualLibDirs()){
        if ( not pn.str().compare( 0, s.size() , s ) )
          return true;
    }

    // check if is in /opt, it may have some layout and we must reply yes
    std::string opt("/opt/");
    if ( not pn.str().compare( 0, opt.size() , opt ) )
      return true;

    return false;
  };

  // checks file, adds dyninfos to m_dlinfos if there are some
  auto checkFile =[this]( const PathName& pn ) -> void {
    Path p = pn;
    if(!p.isValid())
      {
        PathName pnTmp( pn.dir() ) ;
        // dot new files are to ignore, these are not binaries
        if (p.str().find( ".new" , pn.str().size()-4 ) != std::string::npos) return ;
        else if ( pnTmp.base() =="incoming" ) p = pnTmp.dir() + "/" + pn.base() ;
        else if ( p.base() =="incoming" ) return ;
        // place a warning here if file still not exists...
        if ( !p.isValid() ) LogInfo() << "Note: can not read file: " << p << "\n" ;
      }

    if(p.isRegularFile())
      {
        ElfFile elfile(p);
        if( elfile.isBinaryOrLibrary() )
          _elfFiles.push_back(elfile);
      }

  };


  //open the file,
  std::ifstream ins( _path.str().c_str() );
  if ( !ins.good())
    return false;


  std::string line;
  //skip first 5 lines
  for(int i = 0; i< 5; ++i){
      std::getline(ins, line);
      if( ins.eof() ) return false;
  } // if file does not have at least 5 lines , forgett it

  bool handleLine = false;
  while( not ins.eof() )
    {
      std::getline(ins, line);
      if(not handleLine) { // as lone as there is header information, ignore..
        handleLine = line == "FILE LIST:" ? true : false ;
        continue;
      }

      try
        { //need to assemble the pathname, in file without leading dir
          PathName pn("/" + line) ;
          if(isToCheck(pn))
            checkFile(pn);
        }
      catch( const Error& e )
        {
          LogError() << e
              << " (readpkg " << _path.str() << " line:"<< line << ")\n";
        }
      catch( ... )
        {
          LogError() << "Unknown exception (readpkg "
              << _path.str() << " line:"<< line << ")\n";
        }
    }

  return true;
}
//------------------------------------------------------------------------------

ElfFile::Arch
Pkg::getArch() const
{
  if (_elfFiles.empty ())
    {
      return ElfFile::ArchNA;
    }

  return _elfFiles.begin ()->getArch ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}// ns
