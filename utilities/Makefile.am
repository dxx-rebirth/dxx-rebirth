EXTRA_PROGRAMS = hogcreate hogextract mvlcreate mvlextract txb2tex tex2txb

hogcreate_SOURCES = hogcreate.c
hogcreate_MANS = hogcreate.1

hogextract_SOURCES = hogextract.c
hogextract_MANS = hogextract.1

mvlcreate_SOURCES = mvlcreate.c
mvlcreate_MANS = mvlcreate.1

mvlextract_SOURCES = mvlextract.c
mvlextract_MANS = mvlextract.1

txb2tex_SOURCES = txb2tex.c
txb2tex_MANS = txb2tex.1

tex2txb_SOURCES = tex2txb.c
tex2txb_MANS = tex2txb.1

utils: $(EXTRA_PROGRAMS)

EXTRA_DIST = \
	$(hogcreate_MANS) $(hogextract_MANS) \
	$(mvlcreate_MANS) $(mvlextract_MANS) \
	$(txb2tex_MANS) $(tex2txb_MANS)

.PHONY: utils
