AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I macros ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-warnings=fatal \
 --disable-scrollkeeper --disable-update-mime-database


sbin_PROGRAMS	= src/snapp-collector

EXTRA_DIST      = README \
src/Doxyfile \
xml/config.rnc \
xml/example_config.xml \
snapp-collector.8 \
saved_svn_version.txt \
sql/snapp.mysql.sql \
sql/base_example.sql \
snapp-collector.spec \
doc/Makefile \
doc/docbook_doc.xml \
doc/control-protocol.txt \
doc/images/snapp-sql.png \
snapp_configure.pl

man_MANS = snapp-collector.8


src_snapp_collector_SOURCES = src/snapp_config.hpp \
src/snapp_config.cpp \
src/snapp_control.hpp \
src/snapp_control.cpp \
src/xml_load.h\
src/xml_load.c\
src/common.h \
src/util.h \
src/util.c \
src/fnv.h \
src/hash_32.c \
src/rrd_writer.h \
src/rrd_writer.hpp \
src/rrd_writer.cpp \
src/rrd_is_thread_safe.h \
src/host_info.hpp \
src/host_info.cpp \
src/collection_class.hpp \
src/collection_class.cpp \
src/sql-collector.hpp \
src/sql-collector.cpp
