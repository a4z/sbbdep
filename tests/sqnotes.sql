-- select * from sbbdep_depunfiltered where rpkgname == ppkgname and rpkg='libreoffice' order by needed;

/*
SELECT 
   pkgs.fullname AS pkgname, 
   pkgs.name AS pkg, 
   dynlinked.filename AS dynlinked, 
   required.needed ,
   dl2.filename AS filename  
   FROM pkgs 
   INNER JOIN dynlinked ON pkgs.id = dynlinked.pkg_id 
   INNER JOIN required ON dynlinked.id =  required.dynlinked_id
   INNER JOIN dynlinked dl2 ON dl2.soname =  required.needed AND dl2.arch = dynlinked.arch 
   WHERE dl2.dirname IN( 
   SELECT dirname FROM lddirs  
   UNION SELECT dirname FROM ldlnkdirs UNION SELECT dirname FROM ldusrdirs      
   
   ) 
   AND pkgs.name='libreoffice'
   ;
   
   */
   
   
   
   select pkgs.name, dynlinked.filename 
   from dynlinked inner join pkgs on pkgs.id = dynlinked.pkg_id
   where soname = 'libicudata.so.49'
   ;
   
   
   select pkgs.name, d1.filename, d1.soname from dynlinked d1 inner join pkgs on pkgs.id = pkg_id
   where soname in ( select soname from dynlinked where soname is NOT null  group by soname having count(soname) > 1 and arch=64)
   and d1.arch=64 and pkgs.name='libreoffice'
   order by d1.soname
   
   ;   
   
   
   
   
   