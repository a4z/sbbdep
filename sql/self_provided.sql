
CREATE VIEW self_provided AS
SELECT DISTINCT
  dynlinked.pkg_id,
  dynlinked.id,
  pkgs.fullname ,
  dynlinked.filename,
  required.needed,
  d2.id as needed_id,
  d2.filename
  FROM pkgs
    INNER  JOIN dynlinked ON pkgs.id = dynlinked.pkg_id
    INNER  JOIN required ON required.dynlinked_id = dynlinked.id
    INNER  JOIN dynlinked d2 ON d2.soname = required.needed AND d2.pkg_id = dynlinked.pkg_id
;
