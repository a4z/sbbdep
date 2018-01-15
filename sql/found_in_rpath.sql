DROP VIEW  found_in_rpath;
CREATE VIEW found_in_rpath AS
SELECT
  dynlinked.pkg_id,
  dynlinked.id,
  pkgs.fullname ,
  dynlinked.filename,
  required.needed,
  d2.id as needed_id,
  d2.filename as needed_file
  FROM pkgs
    INNER  JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
    INNER  JOIN required ON required.dynlinked_id = dynlinked.id
    INNER  JOIN dynlinked d2 ON d2.soname = required.needed
    INNER  JOIN rrunpath ON dynlinked.id = rrunpath.dynlinked_id
 WHERE d2.dirname = rrunpath.lddir
AND rrunpath.lddir NOT IN (SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs)
AND d2.arch = dynlinked.arch
;

