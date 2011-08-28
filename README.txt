
sbbdep is tool for Slackware and Slackware based distributions like Salixos

 sbbdep takes a snap shoot of installed packages, extracts information about 
 binary runtime dependencies between files and stores these info in a sqlite3 database 
 which is kept in sync.

 the stored info can be used to query binary dependencies between 
 package/packages file/packages package/files file/files in two directions
  what needs files/package 
   and 
  who needs file/package

 through having a static storage sbbdep is fast in querying these information.

 currently query for package/packages and file/packages in both directions are implemented in sbbdep

 For query/generate information sbbdep itself is not a must requirement, 
 the sqlite3 db can be used with other programming languages or sql query tool like 
 the sqlite3 cli or sqliteman.
 A description of the database can be found in README_db.txt.


building sbbdep

Beside boost, libelf and file, which are part of the current Slackware distribution.
sbbdep needs a4z and a4sqlt3 which are both available on bitbucket

an easy way for downloading and building is cloning
https://bitbucket.org/a4z/sbbdep_slk
 if you want the mercurial repository

or download the current state of the source bundle from
https://bitbucket.org/a4z/sbbdep_slk/downloads


more information about build and runtime dependencies can be found at
https://bitbucket.org/a4z/sbbdep_slk
or in the README of the sbbdep_slk source bundle.

