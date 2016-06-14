/*
--------------Copyright (c) 2010-2016 H a r a l d  A c h i t z---------------
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



#ifndef SBBDEP_PATH_HPP_
#define SBBDEP_PATH_HPP_

#include <string>
#include <sys/stat.h>

#include <sbbdep/pathname.hpp>

namespace sbbdep{

class Path : public PathName
{

public:

  enum Type
    {
      INVALID = 0 ,
      ISDIR ,
      ISCHR ,
      ISBLK  ,
      ISREG ,
      ISFIFO ,
      ISLNK ,
      ISSOCK 
    };

  Path();
  Path(const std::string& url);
  Path(const char* url);
  

  Path(Path&& other);
  Path& operator=(Path&& rhs);

  Path(const Path& other);
  Path(const PathName& other);
  
  Path& operator=(const Path& rhs);



  Path& operator=(const std::string& rhs);
  Path& operator=(const char* rhs);
  Path& operator=(const PathName& rhs);
  
  Path& operator+=(const std::string& p);

  bool operator==( const Path& other ) const 
  { return str() == other.str() ; }

  bool operator<( const Path& other ) const 
  { return str() < other.str() ; }  
  
  /// d'tor
  virtual ~Path();
  
  /// returns if path exists
  bool isValid() const { return m_type ; } 
  
  /// returns if path is a regular file
  bool isRegularFile() const { return m_type == ISREG ; } //;
  
  /// returns if path is a folder
  bool isFolder() const { return m_type == ISDIR ; } // ;
  
  /// returns if path is a link
  bool isLink() const { return m_type == ISLNK ; } //;
  
  bool isCharacterSpecial() const { return m_type == ISCHR ; } //;
  
  bool isBlockSpecial() const { return m_type == ISBLK ; } //;
  
  /// returns the Type enum val
  Type getType() const { return m_type; }

  /// returns the Type enum val name as string 
  std::string getTypeString() const ;
  
  /// is user executable
  bool isUserX() const;
  
  const timespec& getLastAccessTime() const ;
  const time_t& getLastModificationTime() const ;
  const time_t& getLastStatusChangeTime() const ;
  
  bool makeRealPath()  ;

  Path getRealPath() const ;

  friend std::ostream& operator<<(std::ostream& os, const Path& p);  
  
private:
  
  Type m_type; 
  struct stat m_stat;
  
  void doStat(); 
  
  void setURL(const std::string& url){ PathName::setURL(url); doStat();}
  
};




}//ns


#endif /* ...PATH_HPP_ */
