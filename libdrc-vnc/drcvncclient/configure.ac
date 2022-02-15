AC_INIT([drcvncclient], [1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
PKG_CHECK_MODULES([libdrc], [libdrc])
PKG_CHECK_MODULES([libvncclient], [libvncclient])
PKG_CHECK_MODULES([sdl2], [sdl2])
PKG_CHECK_MODULES([x264], [x264])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
