dnl Process this file with autoconf to produce configure.
AC_INIT(music, 0.0.0)
AM_INIT_AUTOMAKE

if test "$MPI_CXX" != ""; then
  CXX="$MPI_CXX"
fi

AC_PROG_CXX([mpiCC mpixlcxx mpicxx])
MPI_CXX="$CXX"

AC_MSG_CHECKING([which MPI system we think we are using])
case "$MPI_CXX" in
  mpiCC)
    changequote(<, >)
    if $MPI_CXX -compile_info >/dev/null 2>/dev/null; then
      SYSGUESS=mpich
      CXX="`$MPI_CXX -compile_info | sed -e 's/^\([^ ]*\).*/\1/'`"
    else
      SYSGUESS=openmpi
      CXX="$MPI_CXX"
    fi
    changequote([, ])
    ;;
  mpixlcxx|mpicxx)
    SYSGUESS=bgl
    CXX="$MPI_CXX"
    ;;
  *)
    SYSGUESS=unknown
    ;;
esac
echo "$SYSGUESS"

AC_MSG_CHECKING(MPI_CXXFLAGS)
if test "$MPI_CXXFLAGS" != ""; then
  echo "$MPI_CXXFLAGS"
else
  case "$SYSGUESS" in
    mpich)
      changequote(<, >)
      MPI_CXXFLAGS="`$MPI_CXX -compile_info | sed -e 's/^[^ ]* //;s/ -c / /'`"
      changequote([, ])
      ;;
    openmpi)
      MPI_CXXFLAGS=""    
      ;;
    bgl)
      MPI_CXXFLAGS="-qarch=440 -qtune=440 -qhot -qnostrict"
      enable_shared=no
      ;;
    *)
      AC_MSG_ERROR([
Could not determine proper value for MPI_CXXFLAGS.  Please see README.])
      ;;
  esac
  echo "$MPI_CXXFLAGS"
fi

AC_MSG_CHECKING(MPI_LDFLAGS)
if test "$MPI_LDFLAGS" != ""; then
  echo "$MPI_LDFLAGS"
else
  case "$SYSGUESS" in
    mpich)
      changequote(<, >)
      MPI_LDFLAGS="`$MPI_CXX -link_info | sed -e 's/^[^ ]* //;s/ -c / /'`"
      changequote([, ])
      ;;
    openmpi|bgl)
      MPI_LDFLAGS=""
      ;;
    *)
      AC_MSG_ERROR([
Could not determine proper value for MPI_LDFLAGS.  Please see README.])
      ;;
  esac
  echo "$MPI_LDFLAGS"
fi

if test "$ac_test_CXXFLAGS" != set; then
  if test "$CXXFLAGS" = "-O2"; then
    CXXFLAGS="-O3"
  else
    CXXFLAGS="-g -O3"
  fi
fi

AC_PROG_CC
AC_PROG_LIBTOOL

dnl Checks for rudeconfig:
AC_HEADER_STDC
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([long long])

AC_CHECK_FUNCS([strrchr])

save_CC="$CC"
CC="$MPI_CXX $MPI_CXXFLAGS $MPI_LDFLAGS"
AC_CHECK_FUNCS([rts_get_personality ompi_comm_create])
CC="$save_CC"

AC_SUBST([MPI_CXXFLAGS MPI_LDFLAGS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  test/Makefile
  rudeconfig/Makefile
  utils/Makefile
])

AC_OUTPUT

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl End:
