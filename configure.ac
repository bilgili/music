dnl Process this file with autoconf to produce configure.
AC_INIT(music, 0.0.0)
AM_INIT_AUTOMAKE

AC_CONFIG_SUBDIRS(rudeconfig)

AC_PROG_CXX
if test "$MPI_CXX" != ""; then
  CXX="$MPI_CXX"
else
  dnl Look for MPI C++ compiler
  AC_CHECK_PROGS(MPI_CXX, [mpiCC mpixlcxx mpicxx], "no")
  if test "$MPI_CXX" = "no"; then
    if test "$MPI_CXXFLAGS" = "" -o "$MPI_LDFLAGS" = ""; then
      AC_MSG_ERROR([
Can't find out how to compile with MPI on your system.
Please specify environment variable MPI_CXXFLAGS manually], 1)
    fi
  fi

  AC_MSG_CHECKING([which C++-compiler to use])
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
      echo "$CXX"
      ;;
    mpixlcxx|mpicxx)
      SYSGUESS=bgl
      CXX="$MPI_CXX"
      echo "$CXX"
      ;;
    *)
      SYSGUESS=unknown
      ;;
  esac
fi

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

save_CC=$CC
CC="$MPI_CXX $MPI_CXXFLAGS $MPI_LDFLAGS"
AC_CHECK_FUNCS([rts_get_personality ompi_comm_create])
CC=$save_CC

if test "$ac_test_CXXFLAGS" != set; then
  if test "$CXXFLAGS" = "-O2"; then
    CXXFLAGS="-O3"
  else
    CXXFLAGS="-g -O3"
  fi
fi

AC_PROG_LIBTOOL

AC_SUBST([MPI_CXXFLAGS MPI_LDFLAGS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  test/Makefile
  utils/Makefile
])

AC_OUTPUT

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl End:
