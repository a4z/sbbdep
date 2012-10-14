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

 
 
#include <sbbdep/filemagic.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/path.hpp>

#include <iostream>

#include <omp.h>

namespace sbbdep{



FileMagic::FileMagic()
: m_cookie(0)
{
  
  //MAGIC_NONE
  m_cookie = magic_open(MAGIC_NONE);
  //TOOD, magic_error vieleicht dazunehmen
  if ( !m_cookie ) throw ErrMagic( "!m_cookie" ) ;
  
  magic_load( m_cookie, 0 ) ;
  
  
}
//--------------------------------------------------------------------------------------------------


FileMagic::~FileMagic()
{
  if ( m_cookie ) magic_close(m_cookie);
}
//--------------------------------------------------------------------------------------------------



std::string
FileMagic::getMagicNone(const PathName& pathname)
{

    magic_setflags(m_cookie, MAGIC_NONE);
    return  magic_file( m_cookie, pathname.getURL().c_str() ) ;

}
//--------------------------------------------------------------------------------------------------

std::string 
FileMagic::getMagicMimeType(const PathName& pathname)
{

    magic_setflags( m_cookie, MAGIC_MIME_TYPE );
    return magic_file( m_cookie, pathname.getURL().c_str() ) ;
 
}
//--------------------------------------------------------------------------------------------------

std::string 
FileMagic::getMagicMimeEncoding(const PathName& pathname)
{

    magic_setflags(m_cookie, MAGIC_MIME_ENCODING);
    return  magic_file( m_cookie, pathname.getURL().c_str() ) ;
  
}
//--------------------------------------------------------------------------------------------------

std::string 
FileMagic::getMagicNoCheckElf(const PathName& pathname)
{
    magic_setflags(m_cookie, MAGIC_NO_CHECK_ELF);
    return magic_file( m_cookie, pathname.getURL().c_str() ) ;
      
}
//--------------------------------------------------------------------------------------------------










std::string
FileMagic::getMagicNoneLocked(const PathName& pathname)
{
  std::string retval; 
#pragma omp critical (FileMagicLockedLock) 
  {
    magic_setflags(m_cookie, MAGIC_NONE);
    retval =  magic_file( m_cookie, pathname.getURL().c_str() ) ;
  }
  return retval ;
}
//--------------------------------------------------------------------------------------------------

std::string 
FileMagic::getMagicMimeTypeLocked(const PathName& pathname)
{
  std::string retval; 
#pragma omp critical (FileMagicLockedLock) 
  {
    magic_setflags( m_cookie, MAGIC_MIME_TYPE );
    retval =  magic_file( m_cookie, pathname.getURL().c_str() ) ;
  }
  return retval ; 
}
//--------------------------------------------------------------------------------------------------

std::string 
FileMagic::getMagicMimeEncodingLocked(const PathName& pathname)
{
  std::string retval; 
#pragma omp critical (FileMagicLockedLock) 
  {
    magic_setflags(m_cookie, MAGIC_MIME_ENCODING);
    retval =  magic_file( m_cookie, pathname.getURL().c_str() ) ;
  }
  return retval ;  
}
//--------------------------------------------------------------------------------------------------

std::string 
FileMagic::getMagicNoCheckElfLocked(const PathName& pathname)
{
  std::string retval; 
#pragma omp critical (FileMagicLockedLock) 
  {
    magic_setflags(m_cookie, MAGIC_NO_CHECK_ELF);
    retval =  magic_file( m_cookie, pathname.getURL().c_str() ) ;
  }
  return retval ;      
}
//--------------------------------------------------------------------------------------------------





//--------------------------------------------------------------------------------------------------
} // ns
