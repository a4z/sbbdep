sbbdep (Slack Build Binary Dependencies)
     
     
Slackware Builds and Binaries Dependencies Walker,
sbbdep is a tool for Slackware and Slackware based distributions that traces 
binary runtime dependencies of dynamic linked files.

some simple usage example:

./sbbdep  --whoneeds  --xdl /usr/lib64/libboost_program_options.so
/usr/lib64/libboost_program_options.so.1.49.0 (libboost_program_options.so.1.49.0)
  akonadi-1.9.0-x86_64-1alien
    /usr/bin/akonadi_agent_launcher
    /usr/bin/akonadi_agent_server
    /usr/bin/akonadi_control
    /usr/bin/akonadi_rds
    /usr/bin/akonadictl
    /usr/bin/akonadiserver

this shows us that libboost_program_options.so is required by the package akonadi.


sbbdep works also in the other direction

./sbbdep --xdl --short /usr/lib64/libboost_program_options.so
file /usr/lib64/libboost_program_options.so.1.49.0 needs:
  libc.so.6 found in:
    /lib64/libc-2.15.so( glibc | glibc-solibs )
  libgcc_s.so.1 found in:
    /usr/lib64/libgcc_s.so.1( aaa_elflibs | gcc )
  libm.so.6 found in:
    /lib64/libm-2.15.so( glibc | glibc-solibs )
  libpthread.so.0 found in:
    /lib64/libpthread-2.15.so( glibc | glibc-solibs )
  librt.so.1 found in:
    /lib64/librt-2.15.so( glibc | glibc-solibs )
  libstdc++.so.6 found in:
    /usr/lib64/libstdc++.so.6.0.17( cxxlibs | gcc-g++ )

this shows which packages libboost_program_options.so requires. 


these queries can also be used for packages

./sbbdep  /var/adm/packages/boost-1.49.0-x86_64-3 
aaa_elflibs >= 14.0 | bzip2 >= 1.0.6
aaa_elflibs >= 14.0 | gcc >= 4.7.1_multilib
aaa_elflibs >= 14.0 | zlib >= 1.2.6
cxxlibs >= 6.0.17 | gcc-g++ >= 4.7.1_multilib
glibc >= 2.15_multilib | glibc-solibs >= 2.15_multilib
icu4c >= 49.1.2

or

./sbbdep --whoneeds /var/adm/packages/boost-1.49.0-x86_64-3 
akonadi-1.9.0-x86_64-1alien
boost-1.49.0-x86_64-3
kig-4.10.3-x86_64-1alien

the output format is a package list as slapt-get expects it for dependencies.
(the --short option would generate a list without version number)

passing the --xdl option would list the dependencies for each file of the package.
to keep the samples short this option is skiped here.


If the given argument is a file but not with binary dependencies, sbbdep will
search the package database and show found information.

./sbbdep  /etc/pine.conf 
not a file with binary dependencies: /etc/pine.conf
 try to find other information:
absolute match in /var/adm/packages/alpine-2.02-x86_64-1: /etc/pine.conf



Some more information:

 sbbdep takes a snap shoot of installed packages, extracts information about 
 binary runtime dependencies between files and stores these info in a sqlite3 database 
 which is kept in sync.

 the stored info can be used to query binary dependencies between 
 package/packages file/packages package/files file/files in two directions
  what needs files/package 
   and 
  who needs file/package

 through having a static storage sbbdep is very fast in querying these information.

 currently query for package/packages and file/packages in both directions are 
 implemented.

 For query/generate information sbbdep itself is not a must requirement, 
 the sqlite3 db can be used with other programming languages or sql query tool like 
 the sqlite3 cli or sqliteman.
 A description of the database can be found in README_db.txt.

 for additional information run ./sbbdep --help
 
 
building sbbdep:

sbbdep needs a4z and a4sqlt3 which are both available on bitbucket
an additional build dependency is boost

an easy way for downloading the source disribution including a4z and a4sqlt3 is 
getting the source bundle from
https://bitbucket.org/a4z/sbbdep_slk/downloads

https://bitbucket.org/a4z/sbbdep_slk is this meta repository includes the 
required subrepositories

since sbbdep uses C++11 Slackware 14 or newer ist required.

runtime dependencies from sbbdep can be viewed with sbbdep itself: 

./sbbdep --short --xdl sbbdep 

file /home/slk140/a4work/sbbdep_slk/build/bin/sbbdep needs:
  libc.so.6 found in:
    /lib64/libc-2.15.so( glibc | glibc-solibs )
  libgcc_s.so.1 found in:
    /usr/lib64/libgcc_s.so.1( aaa_elflibs | gcc )
  libgomp.so.1 found in:
    /usr/lib64/libgomp.so.1.0.0( gcc )
  libm.so.6 found in:
    /lib64/libm-2.15.so( glibc | glibc-solibs )
  libpthread.so.0 found in:
    /lib64/libpthread-2.15.so( glibc | glibc-solibs )
  libstdc++.so.6 found in:
    /usr/lib64/libstdc++.so.6.0.17( cxxlibs | gcc-g++ )




    
    
    
    
 