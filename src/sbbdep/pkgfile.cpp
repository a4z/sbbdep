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
 


#include "sbbdep/pkgfile.hpp"
#include "sbbdep/error.hpp"
#include "sbbdep/path.hpp"
#include "sbbdep/log.hpp"
#include "sbbdep/dynlinked.hpp"

#include "a4z/error.hpp"

#include <fstream>
#include <algorithm>



namespace sbbdep {


struct PkgFile::LineHandler
{
  void (LineHandler::*m_lineHandler)( const std::string& ) ;
  
  StringList& m_dirlist ;
  DynLinked& m_dynlinked; // give a ref to this to not always init 
  DynLinkedInfoList& m_dlinfos; 
  
  
  LineHandler(StringList& dirlist, DynLinked& dynlinked, DynLinkedInfoList& dlinfos) 
  : m_lineHandler( &LineHandler::handleHeaderLine)
  , m_dirlist(dirlist)
  , m_dynlinked(dynlinked)  
  , m_dlinfos(dlinfos)
  {
  }//---------------------------------------------------------------------------------------------
  
  
  void handleHeaderLine( const std::string& line )
  {
    if( line == "FILE LIST:" ) m_lineHandler = &LineHandler::handleFileListLine;
  }//---------------------------------------------------------------------------------------------

  void handleFileListLine( const std::string& line )
  {
    // handle "./" , uninteresting, but check what happens if install dest is not /
    if( !(line.length() > 2) ) return;
      
    PathName pn("/" + line) ;
    
    bool checkable = false ;
    for ( StringList::iterator pos=m_dirlist.begin() ; pos!=m_dirlist.end() ; ++pos )
      {
        if ( not pn.getURL().compare( 0, (*pos).size() , *pos ) )  
          {
            checkable=true;
            break;
          }
      }

    
    if ( checkable )
      { // stat the file, check if it is a binary or lib 
        Path p = pn;
        if(!p.isValid())
          {
            PathName pnTmp( pn.getDir() ) ;
            // dot new files are to ignore, these are not binaries
            if (p.getURL().find( ".new" , pn.getURL().size()-4 ) != std::string::npos) return ;
            else if ( pnTmp.getBase() =="incoming" ) p = pnTmp.getDir() + "/" + pn.getBase() ;
            else if ( p.getBase() =="incoming" ) return ;  
            // place a warning here if file still not exists...
            if ( !p.isValid() ) LogInfo() << "Warning: file " << p << " not found\n" ; 
          }
        if( p.isRegularFile())
          {
            if( m_dynlinked.Open( p.getURL() ) )
              {
                m_dlinfos.push_back( DynLinkedInfo() );
                if ( !m_dynlinked.getInfos( m_dlinfos.back() ) ) m_dlinfos.pop_back(); 
              }
            m_dynlinked.Close();
          }
        
      }

    
  }//---------------------------------------------------------------------------------------------
  
  
  void operator()(const std::string& line)
  {
    (this->*m_lineHandler)(line);
  }//---------------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

PkgFile::PkgFile( const PathName& pname )
:Pkg( pname )
{
	

}
//--------------------------------------------------------------------------------------------------

PkgFile::~PkgFile()
{
	
}
//--------------------------------------------------------------------------------------------------

bool 
PkgFile::doLoad( )
{

  std::ifstream ins( m_pathname.getURL().c_str() );
  if ( !ins.good()) return false; 
  
  std::string line;
  
  StringList dirs;
  dirs.insert( dirs.end() , m_bindirs.begin(), m_bindirs.end() ) ;
  dirs.insert( dirs.begin() , m_libdirs.begin(), m_libdirs.end() ) ;  
  DynLinked dynlinked ; // pass dl to not always construct it
  
  LineHandler lineHandler( dirs , dynlinked ,this->m_dlinfos  );

  int cnt = 0; // for skipping the first lines..
  while( not ins.eof() )
    {
      cnt+=1;
      std::getline(ins, line);
      if ( cnt > 5)  // TODO test a package without description or wrong format...
        {
          try
            {
              lineHandler(line);
            }
          catch( const a4z::Err& e )
            {
              LogError() << e << " (readpkg " << m_pathname.getURL() << " line:"<< line << ")\n";
            }
        }
    }
  
  return true; 
  
}
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} // ns
