/*
 --------------Copyright (c) 2013-2013 H a r a l d  A c h i t z---------------
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

#include "featurex.hpp"

#include <sbbdep/lddmap.hpp>
#include <sbbdep/cache.hpp>
#include <sbbdep/log.hpp>


#include <a4sqlt3/sqlcommand.hpp>
#include <a4sqlt3/dataset.hpp>


namespace sbbdep {
namespace cli{

namespace {
  using StingMap = std::map<std::string, std::string> ;
}



void runFx()
{
  

  WriteAppMsg() << " internal test function " << __PRETTY_FUNCTION__ << "\n";

#ifndef DEBUG
  Log::AppMessage() << "not enabled in non debug builds\n";
  return;
#endif

  using namespace a4sqlt3;
  Dataset rs_cnt, rs_files ;

  Cache::get().DB().Execute("SELECT COUNT(*) FROM dynlinked;", rs_cnt);

  Cache::get().DB().Execute("SELECT filename FROM dynlinked;", rs_files);

  int counter = 0;
  for(auto& row : rs_files)
    {
      StingMap ldmap = getLddMap( row.getField(0).getString() )  ;
      LogDebug() << counter << " from " << rs_cnt.getField(0).getInt64() << " - " << ldmap.size() <<   "\n";
      counter++;
    }


}

  
}} // ns
