
sbbdep.cache tables


the database sbbdep uses is simple and strait forward

on the system there are n pacakage installed
table pkgs,  package names found by sbbdep

each package has n dynamic linked binaries or libraries
table dynlinked, dynamic linked binaries and libraries found in package

each dynamic linked binary or library has n required libraries 
table required, names of the libraries needed by a dynamic linked binaries and libraries

there are several places libraries will be searched
tables  lddirs,  ldlnkdirs, rrunpath, ldusrdirs



tables in detail:

TABLE pkgs 
id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,  a simple id, referenced from dynlinked
fullname TEXT NOT NULL,     the full name of the package in the known format name-version-arch-build(tag)
name TEXT NOT NULL,     	the name part of the package name
version TEXT NOT NULL,      the version part of the package name
arch TEXT NOT NULL,         the arch part of the package name
build INTEGER NOT NULL,     the build part of the package name
tag TEXT,    				the tag part of the package name
timestamp INTEGER NOT NULL  file/install time, required for synchronization



table dynlinked  
id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,  a simple id, referenced from required and rrunpath
pkg_id INTEGER NOT NULL,  foreign key of pkgs
filename TEXT NOT NULL,   full file name
dirname TEXT NOT NULL,    dir of file 
basename TEXT NOT NULL,   base name of the file  
soname TEXT ,             the shared name of the library, this is what the linker searches for 
                          if value is NULL, than the file is a binary
arch INTEGER NOT NULL,     32  or 64 bit

pkgs 1 : n dynlinked


table required
id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, a simple id
dynlinked_id INTEGER NOT NULL,    foreign key of dynlinked
needed TEXT NOT NULL,             the soname of dynamic library the dynlinked needs

dynlinked 1 : n required


table lddirs
dirname TEXT PRIMARY KEY NOT NULL contains the /etc/ld.so.conf directories


table ldlnkdirs
dirname TEXT PRIMARY KEY NOT NULL contains directories with libraries that have smylinks in a lddir


table rrunpath
id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL , a simple id
dynlinked_id INTEGER NOT NULL,         foreign key of dynlinked
ldpath TEXT NOT NULL  ,       the rpath or runpath information of a dynamic linked file, 
                              also a place where linker looks for required libs 

dynlinked 1 : n rrunpath


table  ldusrdirs
dirname TEXT PRIMARY KEY NOT NULL
useful for developers or users that have spezial configurations for own installed packages on other library directories exported via LD_LIBRARY_PATH/LD_RUN_PATH

