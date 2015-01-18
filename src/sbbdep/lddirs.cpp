/*
 --------------Copyright (c) 2011-2015 H a r a l d  A c h i t z---------------
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
#include <algorithm>
#include <future>
#include <iterator>
#include <boost/algorithm/string/trim.hpp>

namespace sbbdep {

namespace {

std::vector<std::string>
fillFromLdSoCache ()
{
  std::vector<std::string> retval;

  const std::string cmd = "/sbin/ldconfig -p";
  FILE* popin = popen (cmd.c_str (), "r");
  if (popin)
    {
      char buff [512];

      while (std::fgets (buff, sizeof(buff), popin) != NULL)
        {
          const char* buff_start = buff;
          const char* buff_end = buff + sizeof(buff);

          auto posFound = [&buff](const char* pos) -> bool
            {
              if(pos == buff + sizeof( buff ))
                { // first line might look like
                  // 2683 libs found in cache `/etc/ld.so.cache
                  // it's not good to have a debug message for this
                  //LogDebug()<< "Info: not to parse: " << buff ;
                return false;
              }
            return true;
          };

          const std::string arrow = "=> ";

          using std::begin;
          using std::end;
          using std::binary_search;

          const char* file_begin = std::search (buff_start, buff_end,
                                                begin (arrow), end (arrow))
              + arrow.size ();
          if (not posFound (file_begin))
            {
              continue;
            }

          const std::string lineend = "\n";
          const char* file_end = std::search (file_begin, buff_end,
                                              begin (lineend), end (lineend));
          if (not posFound (file_end))
            {
              continue;
            }

          Path path (std::string (file_begin, file_end));
          path.makeRealPath (); // should always return true,

          retval.emplace_back (path.dir ());
        }

      pclose (popin);
    }

  std::sort (retval.begin (), retval.end ());
  auto last = std::unique (retval.begin (), retval.end ());
  retval.resize (std::distance (retval.begin (), last));
  return retval;

} //-----------------------------------------------------------------------------
} //anno ns

LDDirs::LDDirs ()
{

  auto ldcachedata = std::async (std::launch::async, fillFromLdSoCache);

  _lddirs.emplace_back ("/lib");

  if (Path ("/lib64").isFolder ())
    _lddirs.emplace_back ("/lib64");

  _lddirs.emplace_back ("/usr/lib");

  if (Path ("/usr/lib64").isFolder ())
    _lddirs.emplace_back ("/usr/lib64");

  const char* ldsoconf = "/etc/ld.so.conf";

  Path p (ldsoconf);

  if (p.isValid ())
    _ldSoConfTime = p.getLastModificationTime ();
  else
    throw ErrGeneric ("can not stat /etc/ld.so.conf");

  std::ifstream ldso_conf (ldsoconf);

  if (ldso_conf.is_open ())
    {
      for (std::string line; std::getline (ldso_conf, line);)
        {
          std::size_t commentpos = line.find_first_of ("#");
          if (commentpos != std::string::npos)
            {
              line.erase (commentpos);
            }
          std::string dirname = boost::algorithm::trim_copy (line);
          if (dirname.size ())
            { //  folder path could also be a link
              Path p (dirname);
              p.makeRealPath ();
              if (p.isFolder ())
                {
                  _lddirs.emplace_back (p.str ());
                }
            }
        }
    }
  else
    {
      throw ErrGeneric ("unable to open /etc/ld.so.conf");
    }

  std::sort (_lddirs.begin (), _lddirs.end ());
  auto ldcache = ldcachedata.get ();
#ifdef DEBUG
  SBBASSERT (std::is_sorted (_lddirs.begin (), _lddirs.end ()));
#endif

  std::set_difference (_lddirs.begin (), _lddirs.end (),
                       ldcache.begin (), ldcache.end (),
                       std::inserter (_ldlnkdirs, _ldlnkdirs.begin ()));

}
//------------------------------------------------------------------------------

const LDDirs&
getLDDirs ()
{
  static const LDDirs lddirs;
  return lddirs;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}

