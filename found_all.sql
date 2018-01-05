CREATE VIEW found_all AS 
SELECT
  dynlinked.pkg_id,
  dynlinked.id,
  pkgs.fullname ,
  dynlinked.filename,
  required.needed,
  d2.id as needed_id,
  d2.filename as needed_file
 FROM  pkgs
 INNER JOIN dynlinked on pkgs.id = dynlinked.pkg_id
 INNER JOIN required on dynlinked.id = required.dynlinked_id
 LEFT JOIN rrunpath on dynlinked.id = rrunpath.dynlinked_id
 LEFT JOIN dynlinked d2 on required.needed = d2.soname
 WHERE  d2.arch = dynlinked.arch
AND d2.filename = '/usr/lib64/libboost_program_options.so.1.59.0'
;
