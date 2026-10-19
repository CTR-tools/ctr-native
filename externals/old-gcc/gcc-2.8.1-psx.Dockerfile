FROM ubuntu:focal@sha256:c664f8f86ed5a386b0a340d981b8f81714e21a8b9c73f658c4bea56aa179d54a AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y build-essential gcc gcc-multilib

ENV VERSION=2.8.1
WORKDIR /work
COPY gcc-2.8.1.tar.gz /work/
RUN echo '3b30fbfdf93e628373d90d174243f3267b0eec9ebe792bb64fd15b8828c2ea4c  gcc-2.8.1.tar.gz' | sha256sum -c -
RUN tar xzf gcc-${VERSION}.tar.gz

WORKDIR /work/gcc-${VERSION}

COPY patches /work/patches
RUN sed -i -- 's/include <varargs.h>/include <stdarg.h>/g' *.c

RUN patch -u -p1 obstack.h -i ../patches/obstack-2.8.1.h.patch
RUN patch -u -p1 config/mips/mips.h -i ../patches/mips.patch
RUN patch -su -p1 < ../patches/psx.patch
RUN patch --fuzz=0 -u -p1 < ../patches/psx-gte-delay-policies-2.8.1.patch

RUN ./configure \
    --target=mips-sony-psx \
    --prefix=/opt/cross \
    --with-endian-little \
    --with-gnu-as \
    --disable-gprof \
    --disable-gdb \
    --disable-werror \
    --host=i386-pc-linux \
    --build=i386-pc-linux

RUN touch insn-config.h

RUN make --jobs=2 cc1 CFLAGS="-std=gnu89 -m32 -static"

COPY prebuilt/linux-i386/2.8.1/gcc prebuilt/linux-i386/2.8.1/cpp /work/gcc-${VERSION}/
COPY tests /work/tests
RUN ./cc1 -quiet -O2 /work/tests/little_endian.c && grep -E 'lbu\s\$2,0\(\$4\)' /work/tests/little_endian.s
RUN ./cc1 -quiet -O2 /work/tests/section_attribute.c
RUN ./cc1 -quiet -O2 /work/tests/host_wide_int.c && grep -E '# 0x80000000$' /work/tests/host_wide_int.s
RUN ./cc1 -version</dev/null 2>&1 | grep -- -msoft-float
RUN ./cc1 -version </dev/null 2>&1 | grep -- -msplit-addresses
RUN ./cc1 -version </dev/null 2>&1 | grep -- -mgpopt
RUN sh /work/tests/check_psx_gte.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}

FROM scratch AS export
COPY prebuilt/linux-i386/2.8.1/ /
COPY --from=build /work/gcc-2.8.1/cc1 /cc1
