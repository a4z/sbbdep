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


#ifndef SBBDEP_APPARGS_HPP_
#define SBBDEP_APPARGS_HPP_

#include <string>

namespace sbbdep {

class AppArgs
{
  
  bool m_help;
  std::string m_dbname ;
  std::string m_query;
  std::string m_outfile ;
  bool m_append_versions;
  bool m_nosync;
  
public:
  AppArgs();
  ~AppArgs();
  
  bool Parse( int argc, char** argv ) ;
  
  bool getHelp() const { return m_help ;}
  const std::string& getDBName() const { return m_dbname ; }
  const std::string& getQuery() const { return m_query; }
  const std::string& getOutFile() const { return m_outfile ; }
  bool getAppendVersions() const { return m_append_versions ;}
  bool getNoSync() const { return m_nosync ; }
  
  void PrintHelp();
 
};

}

#endif /* SBBDEP_APPARGS_HPP_ */
