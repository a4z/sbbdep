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

#include <sbbdep/path.hpp>

#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <iostream>

namespace sbbdep {

Path::Path () :
    PathName (),
    m_type (INVALID)
{

}

//------------------------------------------------------------------------------

Path::Path (const std::string& url) :
    PathName (url),
    m_type (INVALID)
{

  doStat ();

}
//------------------------------------------------------------------------------

Path::Path (const char* url) :
    PathName (url),
    m_type (INVALID)
{
  doStat ();
}
//------------------------------------------------------------------------------

Path::Path (const PathName& other) :
    PathName (other),
    m_type (INVALID)
{
  doStat ();
}
//------------------------------------------------------------------------------

Path::Path (const Path& other) :
    PathName (other.str ()),
    m_type (INVALID)
{
  doStat ();
}
//------------------------------------------------------------------------------

Path&
Path::operator= (const Path& rhs)
{
  if (&rhs != this)
    setURL (rhs.str ());

  return *this;
}
//------------------------------------------------------------------------------

Path::Path (Path&& other) :
    PathName (std::move (other.str ())),
    m_type (std::move (other.m_type)),
    m_stat (std::move (other.m_stat))
{

}
//------------------------------------------------------------------------------

Path&
Path::operator= (Path&& rhs)
{
  if (this != &rhs)
    {
      m_type = std::move (rhs.m_type);
      m_stat = std::move (rhs.m_stat);
      PathName::operator =(rhs) ;
    }

  return *this;
}

Path&
Path::operator= (const std::string& rhs)
{
  setURL (rhs);
  return *this;
}
//------------------------------------------------------------------------------

Path&
Path::operator= (const char* rhs)
{
  setURL (rhs);
  return *this;
}
//------------------------------------------------------------------------------

Path&
Path::operator= (const PathName& rhs)
{
  if (&rhs != this)
    setURL (rhs.str ());

  return *this;
}
//------------------------------------------------------------------------------

Path&
Path::operator+= (const std::string& p)
{
  setURL (str() + p);
  return *this;
}
//------------------------------------------------------------------------------

Path::~Path ()
{
}

//------------------------------------------------------------------------------

void
Path::doStat ()
{

  //use lstat wege links..
  if (lstat (str ().c_str (), &m_stat) == -1)
    {
      m_type = INVALID;
      return;
    }

  if (S_ISREG(m_stat.st_mode))
    m_type = Path::ISREG;
  else if (S_ISDIR(m_stat.st_mode))
    m_type = Path::ISDIR;
  else if (S_ISLNK(m_stat.st_mode))
    m_type = Path::ISLNK;
  else if (S_ISCHR(m_stat.st_mode))
    m_type = Path::ISCHR;
  else if (S_ISBLK(m_stat.st_mode))
    m_type = Path::ISBLK;
  else if (S_ISSOCK(m_stat.st_mode))
    m_type = Path::ISSOCK;
  else if (S_ISFIFO(m_stat.st_mode))
    m_type = Path::ISFIFO;
  else
    m_type = INVALID; // just to be sure ..

}
//------------------------------------------------------------------------------

std::string
Path::getTypeString () const
{

  //just a debug function

  if (m_type == Path::ISREG)
    return "ISREG";
  else if (m_type == Path::ISDIR)
    return "ISDIR";
  else if (m_type == Path::ISLNK)
    return "ISLNK";
  else if (m_type == Path::ISCHR)
    return "ISCHR";
  else if (m_type == Path::ISBLK)
    return "ISBLK";
  else if (m_type == Path::ISSOCK)
    return "ISSOCK";
  else if (m_type == Path::ISFIFO)
    return "ISFIFO";
  else
    /*if ( m_type == Path::INVALID )*/return "INVALID";

}
//------------------------------------------------------------------------------

bool
Path::isUserX () const
{
  //if ( m_type == INVALID  ) return false; 
  return (m_stat.st_mode & S_IXUSR) ? true : false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

const timespec&
Path::getLastAccessTime () const
{
  return m_stat.st_atim;
}
//------------------------------------------------------------------------------

const time_t&
Path::getLastModificationTime () const
{
  return m_stat.st_mtime;
}
//------------------------------------------------------------------------------

const time_t&
Path::getLastStatusChangeTime () const
{
  return m_stat.st_ctime;
}
//------------------------------------------------------------------------------

bool
Path::makeRealPath ()
{

  char buf [PATH_MAX];

  char* c = realpath (str ().c_str (), buf);

  if (!c)
    return false;

  setURL (buf);

  return isValid ();

}
//------------------------------------------------------------------------------

Path
Path::getRealPath() const
{
  Path ret = *this ;
  ret.makeRealPath() ;
  return ret ;

}
//------------------------------------------------------------------------------

std::ostream&
operator<< (std::ostream& os, const Path& p)
{
  os << p.str ();
  return os;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}//ns

/*

 struct stat {
 dev_t     st_dev;     // ID of device containing file
 ino_t     st_ino;     // inode number
 mode_t    st_mode;    // protection
 nlink_t   st_nlink;   // number of hard links
 uid_t     st_uid;     // user ID of owner
 gid_t     st_gid;     // group ID of owner
 dev_t     st_rdev;    // device ID (if special file)
 off_t     st_size;    // total size, in bytes
 blksize_t st_blksize; // blocksize for filesystem I/O
 blkcnt_t  st_blocks;  // number of blocks allocated
 time_t    st_atime;   // time of last access
 time_t    st_mtime;   // time of last modification
 time_t    st_ctime;   // time of last status change
 };


 -- Macro: int S_ISDIR (mode_t M)
 This macro returns non-zero if the file is a directory.

 -- Macro: int S_ISCHR (mode_t M)
 This macro returns non-zero if the file is a character special
 file (a device like a terminal).

 -- Macro: int S_ISBLK (mode_t M)
 This macro returns non-zero if the file is a block special file (a
 device like a disk).

 -- Macro: int S_ISREG (mode_t M)
 This macro returns non-zero if the file is a regular file.

 -- Macro: int S_ISFIFO (mode_t M)
 This macro returns non-zero if the file is a FIFO special file, or
 a pipe.  *Note Pipes and FIFOs::.

 -- Macro: int S_ISLNK (mode_t M)
 This macro returns non-zero if the file is a symbolic link.  *Note
 Symbolic Links::.

 -- Macro: int S_ISSOCK (mode_t M)
 This macro returns non-zero if the file is a socket.  *Note
 Sockets::.


 */
