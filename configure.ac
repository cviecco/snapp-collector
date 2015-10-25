#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(snapp-collector, 3.0.7, syseng@grnoc.iu.edu)
#AM_INIT_AUTOMAKE([snapp-collector], [2.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#AC_CONFIG_SRCDIR([snapp-collector.c])
AC_CONFIG_HEADER([src/config.h])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h syslog.h unistd.h pthread.h math.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([floor gettimeofday memset strstr])

# CFLAGS=""

dnl AM_PATH_RRD(, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for ucd-snmp and define UCDSNMP_CFLAGS, UCDSNMP_LDFLAGS and UCDSNMP_LIBS
dnl


AC_DEFUN([AM_PATH_RRD],
[
AC_ARG_WITH(rrd-prefix, [--with-rrd-prefix=PFX Prefix where rrdtool is installed],
            rrd_prefix="$withval",
            rrd_prefix="")

AC_ARG_WITH(rrd-include, [--with-rrd-include=DIR Directory pointing to rrd include files],
            rrd_include="$withval",
            rrd_include="")

AC_ARG_WITH(rrd-lib,
[  --with-rrd-lib=LIB  Directory pointing to rrd library
                          (Note: -include and -lib do override
                           paths found with -prefix)
],
            rrd_lib="$withval",
            rrd_lib="")

RRD_LDFLAGS=""	
RRD_CFLAGS=""
RRD_LIBS="-lrrd_th"
rrd_fail=""

dnl test --with-rrd-prefix
if test "x$rrd_prefix" != "x" ; then
    if test -d "$rrd_prefix" ; then
        if test -d "$rrd_prefix/lib" ; then
            RRD_LDFLAGS="-L$rrd_prefix/lib -L$rrd_prefix/lib64"
        fi
        if test -d "$rrd_prefix/include" ; then
            RRD_CFLAGS="-I$rrd_prefix/include"
        fi
    fi
fi

dnl test --with-rrd-include
if test "x$rrd_include" != "x" ; then
    if test -d "$rrd_include/include" ; then
        RRD_CFLAGS="-I$rrd_include/include"
    elif test -d "$rrd_include" ; then
        RRD="-I$rrd_include"
    fi
fi

dnl test --with-rrd-lib
if test "x$rrd_lib" != "x" ; then
    if test -d "$rrd_lib/lib" ; then
        RRD_LDFLAGS="-L$rrd_lib/lib "
    elif test -d "$rrd_lib" ; then
        RRD_LDFLAGS="-L$rrd_lib"
    fi
fi

CFLAGS="${CFLAGS}    ${RRD_CFLAGS}"
LDFLAGS="${LDFLAGS}  ${RRD_LDFLAGS}"
AC_SUBST(RRD_CFLAGS)
AC_SUBST(RRD_LDFLAGS)
LIBS="${LIBS} ${RRD_LIBS}"

])


##always enable stack checking?
#CFLAGS="${CFLAGS}    -fstack-check"


######RRDTOOL (again)
AC_ARG_WITH(librrd_includes,
    [  --with-librrd-includes=DIR  librrd include directory],
    [with_librrd_includes="$withval"],[with_librrd_includes=no])

AC_ARG_WITH(librrd_libraries,
    [  --with-librrd-libraries=DIR  librrd library directory],
    [with_librrd_libraries="$withval"],[with_librrd_libraries=no])

if test "$with_librrd_includes" != "no"; then
   CFLAGS="${CFLAGS} -I${with_librrd_includes}"
   CPPFLAGS="${CPPFLAGS} -I${with_librrd_includes}"
fi
if test "$with_librrd_libraries" != "no"; then
   LDFLAGS="${LDFLAGS}  -L${with_librrd_libraries}"
fi



#AC_CHECK_HEADER([rrd.h],
#                     [AC_DEFINE([HAVE_RRD_H], [1],
#                        [Define to 1 if you have <rrd.h>.])],
#                     )
#AC_SUBST(HAVE_RRD_H)
#AC_CHECK_HEADER([rrd/rrd.h],
#                     [AC_DEFINE([HAVE_RRD_RRD_H], ,
#                        [Define to 1 if you have <rrd/rrd.h>.])],
#                     )
#AC_SUBST(HAVE_RRD_RRD_H)



AC_CHECK_HEADERS([rrd.h],[],[AC_MSG_ERROR([Missing RRD Tool libraries - try with --with-rrd-prefix=<prefix> to manually specify the directory])])

AC_CHECK_LIB(rrd_th, rrd_get_context, ,[AC_MSG_ERROR([Problem with RRD Libraries])])


dnl #----- check for presence of DBI

AC_ARG_WITH(libdbi_includes,
    [  --with-libdbi-includes=DIR  libdbi include directory],
    [with_libdbi_includes="$withval"],[with_libdbi_includes=no])

AC_ARG_WITH(libdbi_libraries,
    [  --with-libdbi-libraries=DIR  libdbi library directory],
    [with_libdbi_libraries="$withval"],[with_libdbi_libraries=no])

if test "$with_libdbi_includes" != "no"; then
   CFLAGS="${CFLAGS} -I${with_libdbi_includes}"
   CPPFLAGS="${CPPFLAGS} -I${with_libdbi_includes}"
fi
if test "$with_libdbi_libraries" != "no"; then
   LDFLAGS="${LDFLAGS}  -L${with_libdbi_libraries}"
fi

LDBI=""
AC_CHECK_LIB(dbi, dbi_conn_query,, LDBI="no")

if test "$LDBI" = "no"; then
  echo
  echo "   ERROR! DBI library/headers not found, go get it from"
  echo "   http://libdbi.sourceforge.net/download.php"
  echo "   or use the --with-libdbi-* options, if you have it installed"
  echo "   in unusual place"
  exit
fi

##########################

##########
dnl #----- check for presence of PCRE

AC_ARG_WITH(libpcre_includes,
    [  --with-libpcre-includes=DIR  libpcre include directory],
    [with_libpcre_includes="$withval"],[with_libpcre_includes=no])

AC_ARG_WITH(libpcre_libraries,
    [  --with-libpcre-libraries=DIR  libpcre library directory],
    [with_libpcre_libraries="$withval"],[with_libpcre_libraries=no])

if test "$with_libpcre_includes" != "no"; then
   CPPFLAGS="${CPPFLAGS} -I${with_pcre_includes}"
fi
if test "$with_libpcre_libraries" != "no"; then
   LDFLAGS="${LDFLAGS}  -L${with_pcre_libraries}"
fi

AC_CHECK_HEADER([pcre.h],
                     [AC_DEFINE([HAVE_PRCE_H], [1],
                        [Define to 1 if you have <pcre.h>.])],
                     )
AC_SUBST(HAVE_PCRE_H)
AC_CHECK_HEADER([pcre/pcre.h],
                     [AC_DEFINE([HAVE_PCRE_PRCE_H], ,
                        [Define to 1 if you have <pcre/pcre.h>.])],
                     )
AC_SUBST(HAVE_PCRE_PCRE_H)


LPCRE=""
AC_CHECK_LIB(pcre, pcre_compile,, LPCRE="no")

if test "$LPCRE" = "no"; then
  echo
  echo "   ERROR! PCRE library/headers not found, go get it from"
  echo "   http://www.pcre.org/"
  echo "   or use the --with-libpcre-* options, if you have it installed"
  echo "   in unusual place"
  exit
fi


#####################

###section for bigmem
AC_ARG_ENABLE([largecollection],
            [AS_HELP_STRING([--enable-largecollection],
              [enable experimental support for large collections, do NOT use on 32bit systems])],
            [
  CFLAGS="${CFLAGS}    -mcmodel=medium"
  AC_DEFINE([LARGECOLLECTION],[1],[Defined if you want largecollections])
],
            [])


#CFLAGS="${CFLAGS}    -mcmodel=medium"
AC_ARG_ENABLE([iotesting],
              [AS_HELP_STRING([--enable-iotesting],[Disable SNMP requests, for IO testing])
              ],AC_DEFINE([IO_TESTING],[1],[Defined to avoid SNMP]),[])




AC_PATH_PROGS([XML_CONFIG], [xml2-config],[],[${prefix}/bin ${exec_prefix}/bin /usr/bin /usr/local/bin /opt/local/bin])

if test -n "$XML_CONFIG"; then
  vers=`$XML_CONFIG --version 2>/dev/null | sed -e 's/libxml //' | awk 'BEGIN { FS = "."; } { printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
  if test -n "$vers" && test "$vers" -ge 2005000
  then
     LIBXML_LIBS="`$XML_CONFIG --libs`"
     LIBXML_RPATH=
     for args in $LIBXML_LIBS; do
          case $args in
            -L*)
               LIBXML_LDPATH="$LIBXML_LDPATH $args"
               ;;
          esac
     done
     LIBXML_RPATH=`echo $LIBXML_LDPATH | sed -e "s/-L/-R/g"`
     LIBXML_CFLAGS="`$XML_CONFIG --cflags`"
  else
     AC_MSG_WARN([You need at least libxml 2.5.0])
     HELP_SUBDIR=
  fi
fi

CFLAGS="$CFLAGS $LIBXML_CFLAGS"
CPPFLAGS="$CPPFLAGS $LIBXML_CFLAGS"
LDFLAGS="$LDFLAGS $LIBXML_LDFLAGS"
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LDFLAGS)
LIBS="$LIBS $LIBXML_LIBS"


AM_PATH_RRD()
AC_CHECK_HEADERS([libxml2/libxml/xmlmemory.h libxml2/libxml/parser.h],[],[AC_MSG_ERROR([Missing libXML2 libraries])])
AC_CHECK_LIB(xml2, xmlParseFile, [],[AC_MSG_ERROR([Problem with libXML2])])
#AC_CHECK_LIB(rrd_th, rrd_get_context, ,[AC_MSG_ERROR([Problem with RRD Libraries])])


# Check for rrdcached:
AC_ARG_ENABLE([rrdcached],
            [AS_HELP_STRING([--enable-rrdcached],
              [enable experimental support rrdcached NOT recommended for production use])],
            [
  #CFLAGS="${CFLAGS}    -mcmodel=medium"
  #AC_DEFINE([LARGECOLLECTION],[1],[Defined if you want largecollections])
  AC_CHECK_LIB([rrd_th],[rrdc_update],
    
    AC_DEFINE([USE_RRDCACHED],[1],[Defined use RRDCACHED])
    ,
    AC_MSG_ERROR([requested rrdcached but librrd does not support it]))
],

            [])




AC_CHECK_HEADERS([net-snmp/net-snmp-config.h],[],[AC_MSG_ERROR([Missing NET-SNMP Libraries])])


# Checks for libraries.
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [fabs])
AC_CHECK_LIB([pthread], [pthread_create], [], [echo "pthread is required for snapp"
					       exit -1])					     
#this check seems redundant with the code for rrd checks.
#AC_CHECK_LIB([rrd_th], [rrd_update_r], [], [echo "rrdtool >= 1.3.0 is required for snapp"
#					    exit -1])

##the check for snmplib tests for two potential names. snmp is valid for 5.3.x , netsnmp for 5.5.x
AC_CHECK_LIB([snmp],[snmp_sess_open],[],
[AC_CHECK_LIB([netsnmp], [snmp_sess_open], [], [echo "net-snmp is required for snapp"
                                            exit -1])]  )

#another redundant check.
#AC_CHECK_LIB([xml2], [xmlDocGetRootElement], [], [echo "libxml >= 2.5.0 is required for snapp"
#					          exit -1])

# FIXME: Replace `main' with a function in `-lz':
AC_CHECK_LIB([z], [zlibVersion])

###do svn versioning
AC_PATH_PROG(svnversioncommand, svnversion)
if test "X$svnversioncommand" = "X" || test `$svnversioncommand -n '.'` = "exported"; then
AC_DEFINE_UNQUOTED(SVN_VERSION, ["`cat saved_svn_version.txt`"], [repository svn version])
else
AC_DEFINE_UNQUOTED(SVN_VERSION, ["`svnversion -n`"], [repository svn version])
`svnversion -n > saved_svn_version.txt`
fi


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
