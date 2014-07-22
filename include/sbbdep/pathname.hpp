/*
--------------Copyright (c) 2010-2014 H a r a l d  A c h i t z---------------
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


#ifndef SBBDEP_PATHNAME_HPP_
#define SBBDEP_PATHNAME_HPP_


#include <string>



namespace sbbdep{


class PathName
{

public:
  
  PathName();
  PathName(const std::string& url);
  PathName(const char* url);
  
  virtual ~PathName();
  
  PathName(const PathName& other);
  
  
  
  PathName& operator=(const PathName& rhs);
  PathName& operator=(const std::string& rhs);
  PathName& operator=(const char* rhs);
  
  const std::string& getURL() const { return m_url; }
  const std::string& Str() const { return m_url; } // 
  
  std::string getBase() const ;
  std::string getDir() const ;

  bool isEmpty() const { return m_url.empty() ;}
  
  // kann ja auch nur ein datei name sein..
  bool isPath() const;
  
  // beginnt mit ./ or ../
  bool isRelative() const;

  // beginnt mit /
  bool isAbsolute() const ;

  
  operator const char*() const { return m_url.c_str() ; }
  operator const std::string&() const { return m_url ; }
  
  bool operator==( const PathName& other ) const 
  { return m_url == other.m_url ; }

  bool operator==( const std::string& other ) const 
  { return m_url == other ; }
  

  struct LessCompair {
    bool operator() (const PathName& lhs, const PathName& rhs) const {return lhs.getURL() < rhs.getURL();}
  };
  
protected:
  // for encapsulation in Path,..
  virtual void setURL(const std::string& url){m_url=url;}
  
private:
  std::string m_url; 
  
};



}//ns


#endif /* ...PATHNAME_HPP_ */
