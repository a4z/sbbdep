/*
--------------Copyright (c) 2011-2012 H a r a l d  A c h i t z---------------
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


#include "sbbdep/lddirs.hpp"

#include "sbbdep/path.hpp"
#include "sbbdep/dircontent.hpp"
#include "sbbdep/error.hpp"
#include "sbbdep/log.hpp"
#include "sbbdep/dynlinked.hpp"

#include <fstream>
#include <string>
#include <boost/algorithm/string/trim.hpp>



namespace sbbdep
{


LDDirs::LDDirs()
{
}
//--------------------------------------------------------------------------------------------------

LDDirs::~LDDirs()
{
}
//-------------------------------------------------------------------------------------------------- 

const StringSet& 
LDDirs::readLdDirs()
{
  m_lddirs.clear();
  m_lddirs.insert("/lib");
  if(Path("/lib64").isFolder() )m_lddirs.insert("/lib64");
  m_lddirs.insert("/usr/lib");
  if(Path("/usr/lib64").isFolder() )m_lddirs.insert("/usr/lib64");
  
  
  std::string line;
  std::ifstream ldso_conf("/etc/ld.so.conf");

  if (ldso_conf.is_open())
    {
      while (ldso_conf.good())
        {
          std::getline(ldso_conf, line);
          std::size_t commentpos = line.find_first_of("#");
          if( commentpos!=std::string::npos ) line.erase(commentpos);
          std::string dirname = boost::algorithm::trim_copy(line) ;
          if( dirname.size() && Path(dirname).isFolder() ) m_lddirs.insert(dirname) ; // realy required to check if folder?
        }
    }
  else 
    {
      throw ErrStat("unable to read /etc/ld.so.conf");                
    }
  
  
  return m_lddirs; 
}
//--------------------------------------------------------------------------------------------------

const StringSet& 
LDDirs::readLdLinkDirs()
{
  m_ldlnkdirs.clear();
  if(m_lddirs.size()== 0 )readLdDirs();
  
  DynLinked dynlinked;
  
  for (StringSet::const_iterator pos=m_lddirs.begin(); pos!=m_lddirs.end(); ++pos)
    {
      DirContent dir(*pos) ;
      try
        {
          dir.Open();
        }
      catch ( const Error& e )
        {
          LogError()<< "read linked dirs, unable to read " << *pos << std::endl;
          //throw ErrStat("unable to open dir " + *pos);
        }
      
      std::string contend;
      while (dir.getNext(contend))
        {  
          if (contend == "." || contend == "..") continue;
          Path path(dir.getDirName() + "/" + contend);
          if ( path.isLink() )
            {
              path.makeRealPath();
              if (path.isRegularFile())
                {
                  if ( dynlinked.Open( path ) && dynlinked.getType()==DynLinked::Library )
                    {
                      m_ldlnkdirs.insert(path.getDir());
                    }
                  dynlinked.Close();
                }
            }
          
        }      
      
    }
  
  return m_ldlnkdirs; 
}
//--------------------------------------------------------------------------------------------------  


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
}


