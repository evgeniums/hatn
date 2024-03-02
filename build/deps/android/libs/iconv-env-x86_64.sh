#!/bin/sh

IFS='
'

MYARCH=${host_os}-x86_64
ARCH=x86_64
GCCPREFIX=x86_64-linux-android
APILEVEL=$api_level

CFLAGS="
-g
-ffunction-sections
-fdata-sections
-funwind-tables
-fstack-protector-strong
-no-canonical-prefixes
-Wformat
-Werror=format-security
-Oz
-DNDEBUG
-fPIC
$CFLAGS"

CFLAGS="`echo $CFLAGS | tr '\n' ' '`"

LDFLAGS="
-fPIC
-g
-ffunction-sections
-fdata-sections
-Wl,--gc-sections
-funwind-tables
-fstack-protector-strong
-no-canonical-prefixes
-Wformat
-Werror=format-security
-Oz
-DNDEBUG
-Wl,--build-id
-Wl,--warn-shared-textrel
-Wl,--fatal-warnings
-Wl,--no-undefined
-Wl,-z,noexecstack
-Qunused-arguments
-Wl,-z,relro
-Wl,-z,now
-landroid
-llog
-latomic
-lm
$LDFLAGS
"

LDFLAGS="`echo $LDFLAGS | tr '\n' ' '`"

CPP="$CC -E $CFLAGS"

env \
CFLAGS="$CFLAGS" \
CXXFLAGS="$CXXFLAGS $CFLAGS -frtti -fexceptions" \
LDFLAGS="$LDFLAGS" \
CC="$CC" \
CXX="$CXX" \
RANLIB="$RANLIB" \
LD="$CXX" \
AR="$AR" \
CPP="$CPP" \
NM="$NM" \
AS="$AS "\
STRIP="$STRIP" \
"$@"
