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

#include <sbbdep/pkg.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pkgadmdir.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/log.hpp>

#include <fstream>

namespace sbbdep {

// check also #include <sbbdep/lddirs.hpp>
// they contain a bit more
// TODO , check if they are still required, the are also in the database
const Pkg::StringSet&
Pkg::usualBinDirs()
{
  static const StringSet data
    { "/sbin", "/usr/sbin", "/bin", "/usr/bin", "/usr/libexec" };
  return data;
}
const Pkg::StringSet&
Pkg::usualLibDirs()
{
  static const StringSet data
    { "/lib", "/usr/lib", "/lib64", "/usr/lib64" };
  return data;
}

//--------------------------------------------------------------------------------------------------

// former pkg fab, try to work without fmagic, should work, ...
// will possily change in future
Pkg
Pkg::create(Path path)
{


  if( !path.isAbsolute() )
    {
      path.makeAbsolute();
    }
  path.makeRealPath();

  // assume is is a install dest dir,
  // if someting else is given as pathname, like for example "/" ,
  // it is possible senseless and will take a long time , put there is no problem
  if( path.isFolder() )
    {
      return Pkg(std::move(path), PkgType::DestDir);
    }

  if( path.isRegularFile() )
    {
      PkgAdmDir admdir;
      Path admpath(admdir.getDirName());
      admpath.makeRealPath();
      if( path.getDir() == admpath.getURL() )
        {
          return Pkg(std::move(path), PkgType::Installed);
        }

      if( isElfBinOrElfLib(path) )
        {
          return Pkg(std::move(path), PkgType::BinLib);
        }

    }

  return Pkg(std::move(path), PkgType::Unknown);

}

Pkg::Pkg(const Path&& pname, PkgType type)
    : m_path(pname), m_type(type), m_floaded(false)
{

}
//--------------------------------------------------------------------------------------------------


bool
Pkg::Load()
{
  bool retval = false;
  switch (m_type)
    {
    case PkgType::Installed:
      retval =doLoadInstalled();
      break;

    case PkgType::DestDir:
      retval = doLoadDestDir();
      break;

    case PkgType::BinLib:
      retval = doLoadOneBinLib();
      break;

    default:
      retval = false;
      break;
    };
  return retval;
}
//--------------------------------------------------------------------------------------------------

bool
Pkg::doLoadOneBinLib()
{

  ElfFile elfile(m_path);
  if( elfile.isBinaryOrLibrary() )
    m_dlfiles.push_back(elfile);


  return true;
}
//--------------------------------------------------------------------------------------------------

bool
Pkg::doLoadDestDir()
{
  // recursive go through given dir, check for dynlinked in it, if found , added to m_dlinfos
  std::function<void(const std::string&)> checkdir;
  checkdir = [this, &checkdir ]( const std::string& dname ) -> void
  {
    DirContent dcont(dname);
    dcont.Open(); // TODO , could throw
    std::string contend;
    while (dcont.getNext(contend))
      {
        if (contend == "." || contend == "..") continue;

        Path path(dcont.getDirName() + "/" + contend);

        if (path.isFolder())
          {
            checkdir(path.getURL());
          }
        else if (path.isRegularFile())
          {
            ElfFile elfile(path);
            if( elfile.isBinaryOrLibrary() )
              m_dlfiles.push_back(elfile);
          }
      }
  };

  auto dirhandler = [this, checkdir](const std::string& dirname)
    {
    Path path(this->m_path.getURL() + "/" + dirname);
    path.makeRealPath(); // remove // .. dir//usr/ ..
    if (path.isFolder())
      checkdir(path.getURL());
    };

  for(auto& d : Pkg::usualBinDirs())
    dirhandler(d);

  for(auto& d : Pkg::usualLibDirs())
    dirhandler(d);

  return true;
}
//--------------------------------------------------------------------------------------------------

bool
Pkg::doLoadInstalled()
{
  //check if given path name is a bin/lib directory
  auto isToCheck = [this] (const PathName& pn) -> bool {
    for(auto& s : Pkg::usualBinDirs()){
        if ( not pn.getURL().compare( 0, s.size() , s ) )
          return true;
    }
    for(auto& s : Pkg::usualLibDirs()){
        if ( not pn.getURL().compare( 0, s.size() , s ) )
          return true;
    }

    // check if is in /opt, it may have some layout and we must reply yes
    std::string opt("/opt/");
    if ( not pn.getURL().compare( 0, opt.size() , opt ) )
      return true;

    return false;
  };

  // checks file, adds dyninfos to m_dlinfos if there are some
  auto checkFile =[this]( const PathName& pn ) -> void {
    Path p = pn;
    if(!p.isValid())
      {
        PathName pnTmp( pn.getDir() ) ;
        // dot new files are to ignore, these are not binaries
        if (p.getURL().find( ".new" , pn.getURL().size()-4 ) != std::string::npos) return ;
        else if ( pnTmp.getBase() =="incoming" ) p = pnTmp.getDir() + "/" + pn.getBase() ;
        else if ( p.getBase() =="incoming" ) return ;
        // place a warning here if file still not exists...
        if ( !p.isValid() ) LogInfo() << "Note: indexing file " << p << ": file not found\n" ;
      }

    if(p.isRegularFile())
      {
        ElfFile elfile(p);
        if( elfile.isBinaryOrLibrary() )
          m_dlfiles.push_back(elfile);
      }

  };


  //open the file,
  std::ifstream ins( m_path.getURL().c_str() );
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
              << " (readpkg " << m_path.getURL() << " line:"<< line << ")\n";
        }
      catch( ... )
        {
          LogError() << "Unknown exception (readpkg "
              << m_path.getURL() << " line:"<< line << ")\n";
        }
    }

  return true;
}
//--------------------------------------------------------------------------------------------------

ElfFile::Arch
Pkg::getArch() const
{
// TODO , this could be done with more spezial case handling...

  if( m_dlfiles.empty() )
    return ElfFile::ArchNA;

  return m_dlfiles.begin()->getArch() ;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}// ns
