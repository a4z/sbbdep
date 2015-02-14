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


#ifndef SBBDEP_APPARGS_HPP_
#define SBBDEP_APPARGS_HPP_

#include <string>

namespace sbbdep {

class AppArgs
{
  
  int _help;
  std::string _dbname ;
  std::string _query;
  std::string _outfile ;
  int _short_names;
  int _sbbdep_version;
  int _nosync;
  int _require;
  int _whoneeds;
  int _explain_dynlinked;
  int _quiet;
  int _ldd;
  int _lookup;
  int _featureX;
  std::string _featureXArgs;
  std::string _varAdmDir;
  int _bdtree ;
  
public:
  AppArgs();
  ~AppArgs();
  
  bool parse( int argc, char** argv ) ;
  
  bool help() const { return _help ;}
  const std::string& dbname() const { return _dbname ; }
  const std::string& query() const { return _query; }
  const std::string& getOutFile() const { return _outfile ; }
  bool shortNames() const { return _short_names ;}
  bool versions() const { return _sbbdep_version ;}
  bool noSync() const { return _nosync ; }
  bool whoNeeds() const { return _whoneeds ; }
  bool require() const { return _require ; }
  bool xdl() const { return _explain_dynlinked ; }
  bool quiet() const { return _quiet ; }
  bool ldd() const { return _ldd ; }
  bool lookup() const { return _lookup ; }
  bool featureX() const { return _featureX ; }
  const std::string& featureXArgs() const { return _featureXArgs ; }
  const std::string& varAdmDir() const { return _varAdmDir ; }
  bool bdtree() const { return _bdtree ; }
  
  void printHelp();
 
};

}

#endif /* SBBDEP_APPARGS_HPP_ */

