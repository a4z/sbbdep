/*
--------------Copyright (c) 2010-2018 H a r a l d  A c h i t z---------------
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


#include "report.hpp"



#include <sbbdep/cache.hpp>
#include <sbbdep/elffile.hpp>
#include <sbbdep/log.hpp>
#include <sbbdep/path.hpp>
#include <sbbdep/pkg.hpp>
#include <sbbdep/pkgname.hpp>

#include <sbbdep/ldconf.hpp>
#include <sbbdep/error.hpp>


#include <sl3/command.hpp>
#include <sl3/dataset.hpp>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include <map>



namespace sbbdep {
namespace cli{

namespace
{

  using Dataset = sl3::Dataset ;
  using string = std::string ;
  using StringVec = std::vector<string> ;

  using StringMap = std::map<string,string> ;

  using std::begin;
  using std::end;

  const std::string libc6="libc.so." ;

}

ElfFile
findInPaths(ElfFile::Arch arch,
            const std::string& filename, const std::string& envpath )
{
  //const std::string envpath = std::getenv("PATH") ;
  //const std::string envpath = "foo:bar:" ;
  //const std::string envpath = "foo:bar" ;
  //const std::string envpath = "foo::::bar" ;
  //const std::string envpath = ":" ;
  //const std::string envpath = ":foo:bar" ;
  //const std::string envpath = ":foo:" ;
  //const std::string envpath = "" ;

  std::size_t spos = 0, epos = std::string::npos;
  do
    {
      std::string dirname;
      epos = envpath.find (":", spos);
      if (epos != std::string::npos)
        {
          dirname = envpath.substr (spos, epos - spos);
          spos = epos + 1;
        }
      else if (spos < envpath.length ())
        { // get also bar out of "/foo:/bar" but do nothing if just "/foo:"
          dirname = envpath.substr (spos, epos);
        }

      if (!dirname.empty ())
        {
          ElfFile elf(Path (dirname + "/" + filename));
          if (elf.getArch () == arch)
            {
              return elf;
            }
        }
    }
  while (epos != std::string::npos);


  return ElfFile();
}
//------------------------------------------------------------------------------

ElfFile
findInPaths (ElfFile::Arch arch,
             const std::string& filename, const StringVec& dirs)
{
  for (const auto& dir : dirs)
    {
      if (not dir.empty ())
        {
          ElfFile elf(Path (dir + "/" + filename)) ;
          if (elf.getArch () == arch)
            {
              return elf;
            }
        }
    }

  return ElfFile();
}
//------------------------------------------------------------------------------


ElfFile
findInLdSoCache (ElfFile::Arch arch, const std::string& soname)
{
  static auto ldmap = Ldconf::cache() ;
  Ldconf::CacheType::iterator b, e ;
  std::tie(b,e) = ldmap.equal_range (soname) ;

  while (b != e)
    {
      ElfFile elf (Path (b->second));
      if (elf.getArch () == arch)
        {
          return elf;
        }
      ++b;
    }
  return ElfFile() ;
}
//------------------------------------------------------------------------------

ElfFile
findInLdDefaultPathes (ElfFile::Arch arch, const std::string& soname)
{
  StringVec defdirs32{ "/lib","/usr/lib","/usr/local/lib" } ;
  StringVec defdirs64{ "/lib64","/usr/lib64","/usr/local/lib64" } ;

  if (arch == ElfFile::Arch32)
    return findInPaths(arch, soname, defdirs32) ;
  else if (arch == ElfFile::Arch64)
    return findInPaths(arch, soname, defdirs64) ;

  return ElfFile() ;
}
//------------------------------------------------------------------------------


ElfFile
findElfSo(const ElfFile& elf, const std::string& soname)
{

  // see man page, first rpath
  // than ldlibrary
  // than runpath
  // than cache, than folders

  //TODO I miss the append ld_lib_path an rpath.. tls/x86_64:tls:x86_64
  // even if this should not be required,
  // make at least research where they are appended..

  StringVec rrpaths ;

   if (not elf.getRRunPaths().empty ())
     {
       auto resoleRRunpath = [&elf](const std::string& val)
           {
             return replaceLIB(
                 replaceORIGIN (val, elf.getName ().dir ()),
                                elf.getArch ()) ;
           }  ;

       rrpaths.resize (elf.getRRunPaths ().size ());
       std::transform (begin (elf.getRRunPaths ()),
                       end (elf.getRRunPaths ()),
                       begin (rrpaths) ,
                       resoleRRunpath ) ;


     }


  if (elf.hasRPath ())
    {
      auto elfso = findInPaths (elf.getArch(), soname, elf.getRRunPaths ());
      if (elfso.getArch () == elf.getArch())
        return elfso;
    }

  {
    auto ldlibpath = std::getenv ("LD_LIBRARY_PATH") ;
    if (ldlibpath != nullptr)
      {
        auto elfso = findInPaths (elf.getArch (), soname, ldlibpath);
        if (elfso.getArch () == elf.getArch ())
          return elfso;
      }
  }

  if (elf.hasRunPath ())
    {
      auto elfso = findInPaths (elf.getArch(), soname, elf.getRRunPaths ());
      if (elfso.getArch () == elf.getArch())
        return elfso;
    }

  {
    auto elfso = findInLdSoCache (elf.getArch (), soname);
    if (elfso.getArch () == elf.getArch ())
      return elfso;
  }

  return findInLdDefaultPathes(elf.getArch (), soname) ;


}
//------------------------------------------------------------------------------

void
printElf( const std::string& soname,
          const ElfFile& elf,
          int level,
          Cache& cache,
          bool shortNames,
		  bool cycle = false)
{

  SBBASSERT (elf.isElf ()) ;


  std::string indentDep ;

  for (int i = 0; i < level -1; ++i)
    {
      indentDep.append ("|    ");
    }
  std::string indentDepPkgs = indentDep ;
  indentDep.append ("|----" );
  indentDepPkgs.append ("|    " );

  using namespace utils ;


  auto& path = elf.getName ();
  auto  realpath = path.getRealPath () ;

  const auto ds = utils::getPkgsOfFile (cache, realpath, elf.getArch ());
  StringVec pkgs (ds.size ()) ;

  std::string pkgnames ;
  if (ds.size () == 0)
    {
      pkgnames = ("N/A") ;
    }
  else
    {
      using sl3::DbValues ;
      auto getName =  [shortNames](const DbValues& row)
      {
          PkgName name (row [0].getText ());
          return shortNames ? name.name () : name.fullName ();
      } ;

      std::transform (begin (ds), end (ds), begin (pkgs), getName) ;
      pkgnames = joinToString(pkgs, ", ") ;
    }

  if (shortNames)
    {
      LogMsg () << indentDep << soname << " (" << pkgnames << ")" ;
      if (cycle)
    	  LogMsg ()<< indentDepPkgs<< "" << "| !! cycle detected !!";
    }
  else
    {
      if (path.isLink ())
        {
          LogMsg ()<< indentDep << "| "
              << soname << " => "
                << path.str()  << " => " << realpath ;
        }
      else
        {
          LogMsg () << indentDep  << "| "
              << soname << " => "
                << path.str() ;
         }
      pkgnames = joinToString (pkgs, ", ") ;
      LogMsg ()<< indentDepPkgs<< "" << "| (" << pkgnames << ")";
      if (cycle)
    	  LogMsg ()<< indentDepPkgs<< "" << "| !! cycle detected !!";
    }


}
//------------------------------------------------------------------------------

void
printElfs(const ElfFile& elf,
		int level,
		Cache& cache,
		bool shortNames,
		std::set<std::string> parents = {})
{
  SBBASSERT (elf.isElf()) ;

  auto nextparent = parents ;
  nextparent.insert(elf.soName()) ;

  for (const auto& soname : elf.getNeeded ())
    {
// this is used form all bin/libs, so irgnore it here.
// ---libc.so.6 (glibc-solibs, glibc)
//   ---ld-linux-x86-64.so.2 (glibc-solibs, glibc)

      if (soname.size () > libc6.size () &&
          std::equal (begin (libc6), end (libc6), begin (soname)))
        {
          continue ;
        }


      auto soelf = findElfSo (elf, soname);

      if (soelf.getArch () != elf.getArch ())
        {
          LogMsg () << soname << " NOT FOUND" ;
          continue ;
        }


      bool cycle = parents.find(soname) != parents.end() ;
      printElf(soname, soelf, level , cache, shortNames, cycle) ;

      if (cycle)
    	continue ;

      printElfs (ElfFile (soelf.getName ().getRealPath ()),
                 level + 1, cache, shortNames, nextparent) ;

    }

}
//------------------------------------------------------------------------------


void
printLIBC6 (const ElfFile& elf, Cache& cache, bool shortNames)
{
  for (const auto& soname : elf.getNeeded() )
    {
      const std::string c6="libc.so." ;

      if (soname.size () > libc6.size () &&
          std::equal (begin (libc6), end (libc6), begin (soname)))
        {
          auto c6 = findElfSo (elf, soname) ;

          LogMsg() << "used by all:" ;
          if(c6.getArch() == ElfFile::ArchNA)
            {
              LogMsg() << "libc.so* "<< " NOT FOUND" ;
              break ;
            }

          printElf(soname, c6, 2, cache, shortNames) ;

          auto ldlsoname = *begin (c6.getNeeded ()) ;
          auto ldlso = findElfSo (elf, ldlsoname);
          printElf(ldlsoname, ldlso, 3, cache, shortNames) ;

          break ;
        }
    }

}
//------------------------------------------------------------------------------


void
bdTree (Cache& cache, const Pkg& pkg, bool shortNames)
{
  SBBASSERT (pkg.getElfFiles ().size () > 0) ;

  for (const auto& elf : pkg.getElfFiles ())
    {
      LogMsg () << pkg.getPath ();
      printElfs (elf, 1, cache, shortNames);
    }
  printLIBC6 (*begin(pkg.getElfFiles ()), cache, shortNames);


}

}} // ns
