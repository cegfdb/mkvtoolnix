dnl
dnl Debugging, profiling and optimization options
dnl

AC_ARG_ENABLE([debug],
  AC_HELP_STRING([--enable-debug],[compile with debug information (no)]),
  [],
  [enable_debug=no])

AC_ARG_ENABLE([profiling],
  AC_HELP_STRING([--enable-profiling],[compile with profiling information (no)]),
  [],
  [enable_profiling=no])

AC_ARG_ENABLE([optimization],
  AC_HELP_STRING([--enable-optimization],[compile with optimization: -O3 (yes)]),
  [],
  [if test x"$enable_debug" = xyes ; then
     enable_optimization=no
   else
     enable_optimization=yes
   fi])

DEBUG_CFLAGS=""
OPTIMIZATION_CFLAGS=""
PROFILING_CFLAGS=""
PROFILING_LIBS=""

if test x"$enable_debug" = xyes ; then
  DEBUG_CFLAGS="-g -DDEBUG"
fi

if test x"$enable_optimization" = xyes; then
  OPTIMIZATION_CFLAGS="-O3"
fi

if test x"$enable_profiling" = xyes ; then
  PROFILING_CFLAGS="-pg"
  PROFILING_LIBS="-pg"
fi

AC_SUBST(DEBUG_CFLAGS)
AC_SUBST(PROFILING_CFLAGS)
AC_SUBST(PROFILING_LIBS)
AC_SUBST(OPTIMIZATION_CFLAGS)
