if exist %1 goto done
if exist %1.tmp goto done
if not "%2" == "" rcsmerge -r%2 -r%3 %1.c
ren %1.c *.tmp
co %1.c
attrib -r %1.c
diff -b %1.c %1.tmp |more
b %1.c %1.tmp
diff -b %1.c %1.tmp |more
:done
