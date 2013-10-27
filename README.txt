sbbdep (Slack Build Binary Dependencies)
     
     
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

this shows that libboost_program_options.so is required by the package akonadi.


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


If the given argument is a file but not with binary dependencies, sbbdep will
search the package database and show found information.

./sbbdep  /etc/pine.conf 
not a file with binary dependencies: /etc/pine.conf
 try to find other information:
absolute match in /var/adm/packages/alpine-2.02-x86_64-1: /etc/pine.conf



For more information and a detailed overview visit the wiki:
https://bitbucket.org/a4z/sbbdep/wiki/Home

 