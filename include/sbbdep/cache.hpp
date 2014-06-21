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



#ifndef SBBDEP_CACHE_HPP_
#define SBBDEP_CACHE_HPP_


#include <sbbdep/cachedb.hpp>
#include <vector>
#include <string>

namespace sbbdep {

class Cache
{
  
  Cache(const std::string& dbname);

public:

  ~Cache();


  typedef std::vector<std::string> StringVec;
  
  // TODO check, here or elsewhere, that dbname is not empty
  // would open a private db and that gives a wrong error message
  static void open(const std::string& dbname);
  static void close();
  static Cache& get() ;
  
  CacheDB& DB() { return m_db ;}
  

  struct SyncData{
    StringVec removed;
    StringVec installed;
    StringVec reinstalled;
  };

  // lets see which I will prefer to use in future
  SyncData doSync();




private:


  SyncData getSyncData();

  CacheDB m_db;

  static std::unique_ptr<Cache> _instance;
    
  
};



} // ns

#endif /* SBBDEP_CACHE_HPP_ */
