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



#ifndef SBBDEP_SINGLES_HPP_
#define SBBDEP_SINGLES_HPP_

// since for test/trial a whole initialisation is sometimes requiered, but all here untill all ready


#include "sbbdep/cache.hpp"
#include "sbbdep/log.hpp"
#include "sbbdep/filemagic.hpp"
#include "sbbdep/pkfab.hpp"
#include "a4z/singlecollector.hpp"

#include <cstdlib>
#include <string>

namespace sbbdep
{
  void init_all_singles(const char* dbname = 0){
    
    Log::create(); //NOTE; very important, init log on first position..
    
    std::string db;
    if (!dbname) db = std::string(std::getenv("HOME") + std::string("/sbbdep.cache"));
    else db = dbname;
    
    Cache::create( db.c_str() );
    
    FMagSingle::create();
    PkFab::create();    
    
  }
  
  void destroy_all_singles(){
    a4z::SingleCollector::destroy();
  }  
  
}


#endif /* SBBDEP_SINGLES_HPP_ */
