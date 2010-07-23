# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(pward,1,dirkmoermans@gmail.com)
AM_INIT_AUTOMAKE
AC_OUTPUT(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src/proc_impl.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AC_ARG_WITH([proc],
  [  --with-proc-lib=proc
                          used proc library],
  [if test x"$withval" != x ; then
     LPROC="$withval"
   fi],
  [LPROC=proc])

AC_ARG_WITH([proc-hdr],
  [  --with-proc-hdr=proc-hdr
                          used proc header],
  [if test x"$withval" != x ; then
     IPROC="$withval"
   fi],
  [IPROC=proc/readproc.h])

AC_CHECK_LIB($LPROC,openproc)
AC_SUBST(LPROC)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h $IPROC])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strtol])

AC_OUTPUT
