#!/bin/sh
# from https://gist.github.com/unmanned-player/f2421eec512d610116f451249cce5920

cat > /tmp/test.c <<- EOM
#include <stdio.h>

int main(void)
{
	printf("Hello, world!\n");
	return 0;
}
EOM

    CC=$(which $1)
    if [[ -z ${CC} ]]; then
        exit 1
    fi
	tmpf=/tmp/musl.log

	${CC} -o /tmp/test /tmp/test.c

	# Detect Musl C library.
	libc=$(ldd /tmp/test | grep 'musl' | head -1 | cut -d ' ' -f1)
	if [ -z $libc ]; then
		# This is not Musl.
		rm -f ${tmpf} /tmp/test /tmp/test.c
		exit 0
	fi
	$libc >${tmpf} 2>&1
	vstr=$(cat ${tmpf} | grep "Version" | cut -d ' ' -f2)

	v_major=$(echo $vstr | cut -d '.' -f1)
	v_minor=$(echo $vstr | cut -d '.' -f2)
	v_patch=$(echo $vstr | cut -d '.' -f3)

	rm -f ${tmpf} /tmp/test /tmp/test.c

	echo "-D__MUSL__ -D__MUSL_VER_MAJOR__=${v_major} -D__MUSL_VER_MINOR__=${v_minor} -D__MUSL_VER_PATCH__=${v_patch}"
