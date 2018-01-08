DROP VIEW found_all ;
CREATE VIEW found_all AS
SELECT
  dynlinked.pkg_id,
  dynlinked.id,
  pkgs.fullname ,
  dynlinked.filename,
  required.needed,
  required.id,
  d2.id as needed_id,
  d2.filename as needed_file ,
  p2.fullname as needed_pkg
 FROM  pkgs
 INNER JOIN dynlinked on pkgs.id = dynlinked.pkg_id
 INNER JOIN required on dynlinked.id = required.dynlinked_id
 INNER JOIN dynlinked d2 on required.needed = d2.soname
 INNER JOIN pkgs p2 on d2.pkg_id = p2.id
 WHERE  d2.arch = dynlinked.arch

;
