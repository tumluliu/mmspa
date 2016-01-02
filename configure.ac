AC_INIT([mmspa4pg], [2.0.0], [nudtlliu@gmail.com])
#CFLAGS="-arch i386"
CFLAGS="$CFLAGS -I/usr/local/include"
# option for enabling debug mode
#CFLAGS="$CFLAGS -DDEBUG -g3 -O0"
LDFLAGS="$LDFLAGS -L/usr/local/lib"
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AM_PROG_AR
LT_INIT([shared])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CHECK_HEADERS([stdint.h stdlib.h string.h libpq-fe.h])
AC_FUNC_MALLOC
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_CHECK_LIB(pq, PQconnectdb, [], [])
AC_OUTPUT
