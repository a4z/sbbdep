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
  int _append_versions;
  int _sbbdep_version;
  int _nosync;
  int _whoneeds;
  int _explain_dynlinked;
  int _quiet;
  int _ldd;
  int _featureX;
  std::string _featureXArgs;

  
public:
  AppArgs();
  ~AppArgs();
  
  bool Parse( int argc, char** argv ) ;
  
  bool getHelp() const { return _help ;}
  const std::string& getDBName() const { return _dbname ; }
  const std::string& getQuery() const { return _query; }
  const std::string& getOutFile() const { return _outfile ; }
  bool getAppendVersions() const { return _append_versions ;}
  bool getPrintVersions() const { return _sbbdep_version ;}
  bool getNoSync() const { return _nosync ; }
  bool getWhoNeeds() const { return _whoneeds ; }
  bool getXDL() const { return _explain_dynlinked ; }
  bool getQuiet() const { return _quiet ; }
  bool getLdd() const { return _ldd ; }
  bool getFeatureX() const { return _featureX ; }
  const std::string& getFeatureXArgs() const { return _featureXArgs ; }
  
  void PrintHelp();
 
};

}

#endif /* SBBDEP_APPARGS_HPP_ */

