/*
--------------Copyright (c) 2010-2014 H a r a l d  A c h i t z---------------
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


#include <sbbdep/pathname.hpp>




//#ifdef DEBUG
//#include <iostream>
//#endif

namespace sbbdep{


PathName::PathName()
: m_url() 
{
  
}  
//--------------------------------------------------------------------------------------------------

PathName::PathName(const std::string& url)
: m_url(url) 
{
  
}  
//--------------------------------------------------------------------------------------------------

PathName::PathName(const PathName& other)
: m_url(other.getURL())
{
  
}
//--------------------------------------------------------------------------------------------------

PathName::PathName(const char* url)
: m_url(url) 
{

}  
//--------------------------------------------------------------------------------------------------


PathName::~PathName()
{
  
}

//--------------------------------------------------------------------------------------------------

PathName& 
PathName::operator=(const PathName& rhs)
{
  if ( &rhs != this) m_url = rhs.getURL() ;
  return *this;
}
//--------------------------------------------------------------------------------------------------


PathName& 
PathName::operator=(const std::string& rhs)
{
  m_url = rhs;
  return *this;
}
//--------------------------------------------------------------------------------------------------

PathName& 
PathName::operator=(const char* rhs)
{
  m_url = rhs;
  return *this;
}

//--------------------------------------------------------------------------------------------------

/*
path         dirname    basename
"/usr/lib"    "/usr"    "lib"
"/usr/"       "/"       "usr"
"usr"         "."       "usr"
"/"           "/"       "/"
"."           "."       "."
".."          "."       ".."
*/

std::string 
PathName::getBase() const 
{
  
  if( m_url.size() == 0 ) return "";
  
  std::string retval = m_url; 

   // handles "/" and "." 
  if ( retval.size() == 1  ) return retval ;

  std::string::size_type lastSlash = retval.rfind('/' ) ;
  
  // handles "usr" and ".."
  if ( lastSlash ==  std::string::npos ) return retval ;
  
  //make  /usr/lib/ to /usr/lib
  if ( lastSlash ==  retval.size()-1 )
    {
      retval.erase( lastSlash ) ;
      //set last again
      lastSlash = retval.rfind('/' ) ;
    }
  
  // remove chunk name
  retval.erase(0, lastSlash+1) ;
  
  return retval ; 
  
}

//--------------------------------------------------------------------------------------------------
std::string 
PathName::getDir() const
{

  if( m_url.size() == 0 ) return "";
  
  std::string retval = m_url; 
  
   // handles "/" and "." 
  if ( retval.size() == 1  ) return retval ;

  std::string::size_type lastSlash = retval.rfind('/' ) ;
  
  // handles "usr" and ".."
  if ( lastSlash ==  std::string::npos ) 
    {
      retval = ".";
      return retval; 
    }


  //make  /usr/lib/ to /usr/lib
  if ( lastSlash ==  retval.size()-1 )
    {
      retval.erase( lastSlash ) ;
      //set last again
      lastSlash = retval.rfind('/' ) ;
    }
  
  // handle top dirs like /usr
  if ( lastSlash == 0 ) lastSlash = 1;
  
  // remove chunk name
  retval.erase( lastSlash ) ;
  
  return retval ; 
  
  
}
//--------------------------------------------------------------------------------------------------


bool 
PathName::isPath() const
{
  if ( isEmpty() ) return false;
  
  
  const std::string checks[] = {
      std::string("/"), 
          std::string("./"),
              std::string("../")
  };
  // 3
  
  for ( int i = 0 ; i < 3 ; ++i)
    {
      const std::string& check =checks[i] ; 
      if( m_url.compare( 0 ,  check.size(), check  ) == 0 ) return true ;
    }
  
  return false; 
}
//--------------------------------------------------------------------------------------------------


bool 
PathName::isRelative() const
{
  
  if( m_url.compare( 0 ,  2, "./"  ) == 0 ) return true ;
  if( m_url.compare( 0 ,  3, "../"  ) == 0 ) return true ;
  
  return false;
}
//--------------------------------------------------------------------------------------------------

bool 
PathName::isAbsolute() const
{
  return  m_url.compare( 0 ,  1, "/"  ) == 0   ;
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}//ns



