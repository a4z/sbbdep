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
#include <utility>



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

namespace
{
  typedef std::pair<std::string, int> LibInfoType;
  
  struct LibInfoCompair {
    bool operator() (const LibInfoType& lhs, const LibInfoType& rhs) const
      {
        int cmp = lhs.first.compare( rhs.first ) ;
        if (cmp < 0 ) return true;
        else if ( cmp > 0 ) return false;   
        else return lhs.second < rhs.second ;
      }
  };
  
  typedef std::set<LibInfoType, LibInfoCompair> LibInfoSet;

}

void 
DepFileWriter::generate(const Pkg& pkg, std::ostream& outstm)
{
  
  LibInfoSet requiresInfo , providesInfo ;
  
  DynLinkedInfoList::const_iterator cIter = pkg.getDynLinkedInfos().begin();
  for( ; cIter != pkg.getDynLinkedInfos().end(); ++cIter)
    {
      for(StringList::const_iterator pos = cIter->Needed.begin(); pos != cIter->Needed.end(); ++pos)
        {
          requiresInfo.insert(std::make_pair(*pos,cIter->arch)) ;
        }
      
      if ( cIter->soName.size() ) providesInfo.insert(std::make_pair(cIter->soName,cIter->arch)) ;
    }
  
  // remove libs that are within this pkg
  for (LibInfoSet::iterator pos = providesInfo.begin(); pos != providesInfo.end(); ++pos)
    {
      requiresInfo.erase(*pos) ;
    }
  
  
  StringSet deps;
  StringSet notfound;
  PkOfFile searcher;

  for( LibInfoSet::iterator pos = requiresInfo.begin(); pos!= requiresInfo.end(); ++pos )
    {
      StringList pkgsrequired;
      searcher.search(pos->first , pos->second, pkgsrequired) ;
      if( pkgsrequired.size()== 0 ) notfound.insert(pos->first);
      else deps.insert(  joindep2string(pkgsrequired, m_addVersion) ) ;      
    }  
  
  std::string seperator = m_addVersion ? "\n" : ", "; 
  std::ostream_iterator< std::string > oIt (outstm, seperator.c_str() );
  // avoid tailing seperator
  StringSet::iterator last = --(deps.end());
  std::copy ( deps.begin(), last, oIt );
  outstm << *last ;
  for (StringSet::iterator pos = notfound.begin();pos != notfound.end();++pos )
    {
      LogError()<< pkg.getPathName() <<" ! not found: " << *pos <<"\n" ;
    }
  
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

void 
DepFileWriter::who_requires(const Pkg& pkg, std::ostream& outstm)
{

  StringSet requiredby ;  
  
  PkOfFile searcher;
  
  DynLinkedInfoList::const_iterator dliIter = pkg.getDynLinkedInfos().begin();
  for( ; dliIter != pkg.getDynLinkedInfos().end(); ++dliIter)
    {  
      StringList result;
      searcher.searchRequiredBy( dliIter->soName, dliIter->arch , result) ;
      requiredby.insert(result.begin(), result.end());
    }
  
  requiredby.erase( pkg.getPathName().getBase() ) ;
  
  for(StringSet::iterator pos = requiredby.begin(); pos != requiredby.end(); ++pos)
    {
      outstm << *pos << "\n";
    }
  outstm << std::flush; 
      
}

//--------------------------------------------------------------------------------------------------
}
