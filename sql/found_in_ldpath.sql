-- DROP VIEW  found_in_ldpath;
-- CREATE VIEW found_in_ldpath AS
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
WHERE d2.dirname IN (SELECT dirname FROM lddirs UNION SELECT dirname FROM ldlnkdirs)
AND d2.arch = dynlinked.arch
;
