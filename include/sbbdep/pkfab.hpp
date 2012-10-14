/*
--------------Copyright (c) 2010-2012 H a r a l d  A c h i t z---------------
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




#ifndef SBBDEP_PKFAB_HPP_
#define SBBDEP_PKFAB_HPP_


#include <map>
#include <a4z/single.hpp>
#include <sbbdep/pathname.hpp>


namespace sbbdep {

//untill i have a better plan and for test first version use this way..

class PathName; 
class Pkg; 


/**
 * creates holds and destroys Pkgs
 * 
 */
class PkFab : public a4z::Single<PkFab>
{

  friend class a4z::Single<PkFab>;
  PkFab();
  ~PkFab();
  
  
  struct PKMapCompair {
    bool operator() (const PathName* lhs, const PathName* rhs) const {return lhs->getURL() < rhs->getURL();}
  };
  typedef std::map< const PathName* , Pkg* , PKMapCompair > PKMap; 
  
  PKMap m_pkmap; 
  
public:
  
   /** creates a pkg from the path
    * if pkg is already loaded it retunrns the loaded one unless replaceLoaded is passed as ture 
    */    
   Pkg* createPkg( const PathName& pn, bool replaceLoaded = false ); 
   //load pkg kann/muss error schmeissen wenn problem,
   //get pkg func mit * return* damit wenn nicht geladen informiert wird??
   
   // returns if a pkg is already loaded
   bool havePkg( const PathName& pn ) ;
   
   // unloads (deletes a pkg )
   bool unloadPkg( const PathName& pn ) ;
   
private:
   void addPgk(Pkg* pkg);
   
  
};

}

#endif /* SBBDEP_PKFAB_HPP_ */
