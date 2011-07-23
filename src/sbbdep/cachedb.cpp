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


#include "sbbdep/cachedb.hpp"
#include "sbbdep/cachesql.hpp"

#include <cstdlib>
#include <sqlite3.h>


namespace sbbdep {



CacheDB::CacheDB()
: a4sqlt3::Database(std::string(std::getenv("HOME")) + std::string("/sbbdep.cache")) //TODO wenn test vorbei nach .config/.. oder sonst wo
{
 

}
//--------------------------------------------------------------------------------------------------

CacheDB::CacheDB( const std::string& name )
: a4sqlt3::Database( name )
{
 

}
//--------------------------------------------------------------------------------------------------



CacheDB::~CacheDB()
{
 
}
//--------------------------------------------------------------------------------------------------

bool
CacheDB::Open()
{

  int flags = SQLITE_OPEN_READWRITE ; // thread will be handled outside only  
  bool retval = a4sqlt3::Database::Open(flags );
  
  CacheSQL::register_replaceOrigin_function(m_sql3db);
  
  return retval;
}
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
} // ns
