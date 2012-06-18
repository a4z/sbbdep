/*
--------------Copyright (c) 2012-2012 H a r a l d  A c h i t z---------------
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



#include <a4z/testsuitebuilder.hpp>

#include "sbbdep/dircontent.hpp"

#include <functional>
#include <string>
#include <vector>

using namespace sbbdep;

class PkgAdmDir{

  static bool DefaultIgnorFilter(const std::string& f){

    if(f[0] == '.' ) return true;
    if(*(f.rbegin()) == '~') return true;
    if(f[0] == '#' && *(f.rbegin()) == '#') return true;

    return false;
  }

public:

  typedef std::function<bool(const std::string&, const std::string&)> DirContentCall;
  typedef std::function<bool(const std::string&)> IgnorFilterCall;



  PkgAdmDir(  ):m_dir("/var/adm/packages") , m_ignoreFilter(&DefaultIgnorFilter)
  {
    m_dir.Open();
  }
  ~PkgAdmDir(){
    if(m_dir.isOpen()) m_dir.Close();
  }



  void apply(DirContentCall call)
  {
    std::string fname;
    while( m_dir.getNext(fname) ){

        if( m_ignoreFilter(fname) )
          continue;

        if( !call(m_dir.getDirName(), fname) )
          break;

    }

  }

private:
  DirContent m_dir;

  IgnorFilterCall m_ignoreFilter;

};


void RunDefault()
{
  PkgAdmDir admdir;

  typedef std::vector<std::string> StringVec;

  StringVec fnames;
  int counter =0;

  auto func  = [&fnames, &counter](const std::string& d , const std::string& f) -> bool {
    fnames.push_back(f) ;
    counter+=1;
    return counter < 5;
  };

  admdir.apply(func) ;

  BOOST_CHECK_EQUAL( counter , 5 ) ;
  BOOST_CHECK( fnames.size() == 5 );

}



struct VarAdmDirSuite : public a4z::TestSuiteBuilder< >
{

  void
  assambleCases()
  {
    A4Z_TEST_ADDCASEFUNC( RunDefault );
  }

};
A4Z_TEST_CHECK_IN ( VarAdmDirSuite , varadmdir );






