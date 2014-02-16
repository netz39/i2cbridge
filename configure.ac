
AC_PREREQ([2.69])
AC_INIT([i2cbridge], m4_esyscmd_s(echo 1.$(git log|grep "i2cbridge:"|wc -l)),
    [lordyomin@hivegate.net], [i2cbridge], [http://github.com/netz39/netz39_rollladensteuerung])
AM_INIT_AUTOMAKE([-Wall foreign -Wno-portability])
AC_CONFIG_SRCDIR([src/i2cbridge.c])
AC_CONFIG_HEADERS([config.h])

# Checks for libraries.
AC_CHECK_LIB([wiringPi], [wiringPiI2CSetup], [FOO=], AC_MSG_ERROR([wiringPi lib not found]))

# Checks for header files.
AC_CHECK_HEADERS([wiringPi.h arpa/inet.h fcntl.h netdb.h stdint.h stdlib.h string.h sys/socket.h unistd.h],
    [], AC_MSG_ERROR([header file not found]))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset mkdir socket strchr strerror strspn])

CFLAGS="$CFLAGS -Wall -Wpedantic -std=c99"

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
