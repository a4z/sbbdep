-- DROP VIEW  found_as_link;
CREATE VIEW found_as_link AS
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
WHERE EXISTS(SELECT dynlinked_id FROM ldlinks WHERE ldlinks.dynlinked_id = d2.id)
AND d2.arch = dynlinked.arch
;
