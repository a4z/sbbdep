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


#include "sbbdep/depfilewriter.hpp"

#include "sbbdep/pkg.hpp"
#include "sbbdep/pkoffile.hpp"

#include "sbbdep/dynlinkedinfolist.hpp"

#include "sbbdep/stringlist.hpp"
#include "sbbdep/stringset.hpp"
#include "sbbdep/pkgname.hpp"
#include "sbbdep/log.hpp"

#include "a4z/error.hpp"

#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <sstream>



namespace sbbdep {



namespace {

  std::string joindep2string( StringList& slist, bool addVersion  )
  {
    // use a set for sorting, 
    if( slist.size() > 1) slist.sort();

    std::string retVal;
    for(StringList::iterator pos = slist.begin(); pos!=slist.end();++pos)
      {
        if( pos  != slist.begin() ) retVal+=" | " ;
        PkgName n(*pos) ;
        retVal+= n.Name() ;
        if (addVersion) retVal+= " >= " + n.Version()  ; 
      }
    return retVal; 

  }
  
  /*
  struct IsNotFound
  {
    PathNameList& m_notFounds;
    IsNotFound(PathNameList& notFounds):m_notFounds(notFounds){}
    
    bool operator()(const PathName& pn)
    {
      return std::find(m_notFounds.begin(), m_notFounds.end(), pn ) != m_notFounds.end() ; 
    }
  };
  */

}



DepFileWriter::DepFileWriter(bool addVersion)
: m_addVersion(addVersion)
{

}
//--------------------------------------------------------------------------------------------------


DepFileWriter::~DepFileWriter()
{
  
}
//--------------------------------------------------------------------------------------------------



void 
DepFileWriter::generate(const Pkg& pkg, std::ostream& outstm)
{
  
  StringSet requires , provides;  
  
  int arch = 0;
  
  DynLinkedInfoList::const_iterator cIter = pkg.getDynLinkedInfos().begin();
  for( ; cIter != pkg.getDynLinkedInfos().end(); ++cIter)
    {
      requires.insert( cIter->Needed.begin(), cIter->Needed.end()  ) ;
      if ( cIter->soName.size() ) provides.insert(cIter->soName) ;
      
      if ( !arch ) arch = cIter->arch ;
      else
        { // will this ever happen ? 
          if( arch != cIter->arch ) 
            throw a4z::ErrorNeverReach(" multiple arch dedected in " + pkg.getPathName().Str() ) ; 
        }
      
    }
  // remove libs that are within this pkg
  for( StringSet::iterator pos = provides.begin(); pos!= provides.end(); ++pos )
    {
      requires.erase(*pos) ;
    }
  
  
  
  struct Collector{
    StringSet deps; 
    StringSet notfound;
    void collect(StringSet& d , StringSet& nf)
    {
#pragma omp critical (DepFileWriterCollect)
      {
        deps.insert(d.begin(), d.end());
        notfound.insert(nf.begin(), nf.end());
      }
    }
  };
  struct Collect{
    StringSet deps;
    StringSet notfound;
    Collector& collector; 
    Collect(Collector& collector_):collector(collector_){}
    ~Collect(){ collector.collect(deps, notfound) ;} 
  };
  
  Collector deppkgs;
  //Collect collect(deppkgs);  // for going parallel

  PkOfFile searcher;
  
  for( StringSet::iterator pos = requires.begin(); pos!= requires.end(); ++pos )
    {
      Collect collect(deppkgs);  // fake first private..
      StringList pkgsrequired;
      searcher.search(*pos , arch, pkgsrequired) ;
      if( pkgsrequired.size()== 0 ) collect.notfound.insert(*pos);
      else collect.deps.insert(  joindep2string(pkgsrequired, m_addVersion) ) ;
      //else collect.deps.insert(pkgsrequired.begin(), pkgsrequired.end());
      
    }  
  
  std::string seperator = m_addVersion ? "\n" : ", "; 
  std::ostream_iterator< std::string > oIt (outstm, seperator.c_str() );
  std::copy ( deppkgs.deps.begin(), deppkgs.deps.end(), oIt );
  
  
  
}
//--------------------------------------------------------------------------------------------------

void 
DepFileWriter::generate_log(const Pkg& pkg, Log::ChannelType& lc)
{
  std::stringstream ss; 
  generate(pkg, ss);
  lc << ss.str() << "\n";
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}
