/*
--------------Copyright (c) 2011-2014 H a r a l d  A c h i t z---------------
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


#include <sbbdep/lddirs.hpp>

#include <sbbdep/path.hpp>
#include <sbbdep/dircontent.hpp>
#include <sbbdep/error.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/elffile.hpp>

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

auto
LDDirs::readLdDirs() -> const LDDirs::StringSet&
{
  m_lddirs.clear();
  m_lddirs.insert("/lib");
  if(Path("/lib64").isFolder() )m_lddirs.insert("/lib64");
  m_lddirs.insert("/usr/lib");
  if(Path("/usr/lib64").isFolder() )m_lddirs.insert("/usr/lib64");

  std::ifstream ldso_conf("/etc/ld.so.conf");

  if( ldso_conf.is_open() )
    {
      for( std::string line ; std::getline(ldso_conf, line) ; )
        {
          std::size_t commentpos = line.find_first_of("#") ;
          if( commentpos != std::string::npos )
            line.erase(commentpos) ;
          std::string dirname = boost::algorithm::trim_copy(line) ;
          if( dirname.size() )
            { //  folder path could also be a link
              Path p(dirname) ;
              p.makeRealPath() ;
              if( p.isFolder() )
                m_lddirs.insert(p.getURL()) ;
            }
        }
    }
  else
    {
      throw ErrGeneric("unable to read /etc/ld.so.conf") ;
    }
  
  
  return m_lddirs; 
}

int64_t
LDDirs::getLdSoConfTime()
{
  Path p("/etc/ld.so.conf");

  if(p.isValid())
    return p.getLastModificationTime();

  return 0 ;
}

//--------------------------------------------------------------------------------------------------

namespace
{


void fillFromLdSoCache(std::set<std::string>& s,
                       const std::set<std::string>& ignore)
{

  const std::string cmd = "/sbin/ldconfig -p";
  FILE* popin = popen(cmd.c_str(), "r");
  if( popin )
    {
      char buff[512];
      //TODO  first line always has some other infos..

      while (std::fgets(buff, sizeof( buff ), popin) != NULL)
        {
          const char* buff_start = buff;
          const char* buff_end = buff + sizeof( buff ) ;

          auto posFound = [&buff](const char* pos) -> bool{
            if(pos == buff + sizeof( buff ))
              { // TODO first line might look like
                // 2683 libs found in cache `/etc/ld.so.cache
                // it's not good to have a debug message for this
                LogDebug()<< "Info: not to parse: " << buff << "\n";
                return false;
              }
            return true;
          };
/*
          const char* soname_begin = std::find_if( buff_start,  buff_end,
              [](const char c)->bool{ return c != '\t' ; }
              );
          if (not posFound(soname_begin))
              continue;


          const char* soname_end = std::find_if( soname_begin,  buff_end,
              [](const char c)->bool{ return c == ' '; }
          );
          if (not posFound(soname_end))
              continue;
*/

          const std::string arrow = "=> ";
          //const char* file_begin = std::search( soname_end, buff_end,
          const char* file_begin = std::search( buff_start, buff_end,
              std::begin(arrow), std::end(arrow)) + arrow.size();
          if (not posFound(file_begin))
              continue;

          const std::string lineend = "\n";
          const char* file_end = std::search( file_begin, buff_end,
              std::begin(lineend), std::end(lineend)) ;
          if (not posFound(file_end))
              continue;

         // std::cout << "here: '" << std::string(soname_begin, soname_end) << "'" ;
         // std::cout << " to '" << std::string(file_begin, file_end) << "'" ;

          Path path(std::string(file_begin, file_end));
          path.makeRealPath(); // should alswayr return ture,
          // TODO care about this...
          //if (path.isRegularFile() && isElfLib( path ) ) // this should not be required anymore

          if(ignore.find(path.getDir())== ignore.end())
            s.insert(path.getDir());

        }


      pclose(popin);
    }

}
}


/*
 * get all directory names that have files with link in an lddir....
 */
auto
LDDirs::readLdLinkDirs() -> const LDDirs::StringSet&
{
  if(m_lddirs.empty()) // ensure loading cause need it later to filter what already exists
    readLdDirs();

  m_ldlnkdirs.clear();

  fillFromLdSoCache(m_ldlnkdirs, m_lddirs); // fill - ignore
  return m_ldlnkdirs ;

// TODO , remove the dead code below or use it just in case of problems with fillFromLdSoCache

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
              std::string orig = path.Str();
              path.makeRealPath();
              if (path.isRegularFile() && isElfLib( path ) )
                { // filter out what is a regular ld dir , TODO since this is a set lookup not reuqired
                  if(m_lddirs.find(path.getDir())== m_lddirs.end())
                    m_ldlnkdirs.insert(path.getDir());

                  // TODO , why did I do this ? only for debug ??
                  if(path.getDir()=="/usr/bin" || path.getDir()=="/usr/sbin")
                    LogInfo()<<"in bin: " << orig << "->" << path.getURL() << std::endl;
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


