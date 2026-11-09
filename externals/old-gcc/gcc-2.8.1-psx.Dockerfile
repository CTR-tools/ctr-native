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
RUN patch --fuzz=0 -u -p1 < ../patches/psx-frame-policies-2.8.1.patch
RUN patch --fuzz=0 -u -p1 < ../patches/generic-unreachable-2.8.1.patch
RUN patch --fuzz=0 -u -p1 < ../patches/psx-scratch-call-2.8.1.patch
RUN echo 'a2221b5a05bf81f3797b20725e901422e52579681ba5b9b1e95a60767154cd37  ../patches/psx-ram-load-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 -u -p2 < ../patches/psx-ram-load-2.8.1.patch
RUN echo '698ebf5719f8a6ac8a594af894da1dee94400826f04263ee84e53dc2c1d27dcb  ../patches/psx-continuations-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/psx-continuations-2.8.1.patch
RUN echo 'f6ad3c811fa75bc93a63755439b2a806b0344013bbe837db46fd6cca1312b1c0  ../patches/psx-animation-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/psx-animation-2.8.1.patch
RUN echo '341a6892bc34d6470c40809f5d748d59aaac7398b5a379026f4c8e633195950e  ../patches/psx-codegen-policies-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/psx-codegen-policies-2.8.1.patch
RUN echo 'd9db3535d1fe3b4641f6369b544d76f433eb581b2c559375f36849da88467dc0  ../patches/generic-smul-high-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/generic-smul-high-2.8.1.patch
RUN echo 'ddb7cd2bb75d0d635e8385016fbe171be76ce9d9261478908a024b89b36cb621  ../patches/generic-loop-optimize-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/generic-loop-optimize-2.8.1.patch
RUN echo '34aa7a10a85f6aec8a1d9eb1df5dd11ea877cab012db1dfd6856704e3737b22f  ../patches/mips-t9-constraint-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/mips-t9-constraint-2.8.1.patch
RUN echo 'abce0a83ad019cca09a51e73800107fd81d2aec093844a156506c01aca2d4390  ../patches/mips-static-frame-alias-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/mips-static-frame-alias-2.8.1.patch
RUN echo 'ed2765cc6e4b978649cb5522c275d7adfd2a62b72cade7e900dc6356f579ed77  ../patches/mips-ordered-save-direction-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/mips-ordered-save-direction-2.8.1.patch
RUN echo 'a9fb7f24a39142f22fc8b8441c0b6c81ba7134302b415c07c33c8c94cdc8319a  ../patches/generic-scheduler-2.8.1.patch' | sha256sum -c -
RUN patch --batch --forward --fuzz=0 --read-only=warn --no-backup-if-mismatch -u -p1 < ../patches/generic-scheduler-2.8.1.patch
RUN echo '5a476128a14964acfcd31f047c2164c37f3bbb7473946d31b4a3a5be08417285  ../patches/generic-local-register-hint-2.8.1.patch' | sha256sum -c -
RUN chmod u+w c-common.c local-alloc.c Makefile.in && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/generic-local-register-hint-2.8.1.patch
RUN echo '9a07f18d2fd45363dc3856b755a2a9c702685181dcd1e11056cff3748341f4ea  ../patches/mips-nonfaulting-sh-return-2.8.1.patch' | sha256sum -c -
RUN chmod u+w config/mips/mips.c config/mips/mips.h config/mips/mips.md && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/mips-nonfaulting-sh-return-2.8.1.patch
RUN echo 'beb2f8d8fcfc0e9ecc10ced3da1ff202969041c5e98914299c9bc835adcc1156  ../patches/generic-call-setup-tie-2.8.1.patch' | sha256sum -c -
RUN chmod u+w flags.h sched.c toplev.c && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/generic-call-setup-tie-2.8.1.patch

RUN echo '7a7589c325964172ff16ae4b667cfd636fff3780f46ab22c6cda0ed6f9ef66ff  ../patches/mips-private-frame-sw-jump-2.8.1.patch' | sha256sum -c -
RUN chmod u+w config/mips/mips.c config/mips/mips.h && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/mips-private-frame-sw-jump-2.8.1.patch

RUN echo 'f34e266d526290cd9bcdc0c4080aa1069ad28e3cbd665bd384efc58aa74364eb  ../patches/generic-delay-slot-target-first-2.8.1.patch' | sha256sum -c -
RUN chmod u+w flags.h toplev.c reorg.c && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/generic-delay-slot-target-first-2.8.1.patch

RUN echo '3ebceed3f5cb7fe7951c15bcfde4241d78b743f564975ae9d2fdc6f87eec397d  ../patches/generic-continuation-constants-2.8.1.patch' | sha256sum -c -
RUN chmod u+w flags.h toplev.c sched.c jump.c Makefile.in && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/generic-continuation-constants-2.8.1.patch
RUN echo '19826d695779c20b098da1d3b0b82f0a0ff096086bafccdaab1dcfb60abcf70f  ../patches/generic-spill-home-2.8.1.patch' | sha256sum -c -
RUN chmod u+w c-common.c function.c reload1.c rtl.h Makefile.in && patch --batch --forward --fuzz=0 --no-backup-if-mismatch -u -p1 < ../patches/generic-spill-home-2.8.1.patch

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
RUN sh /work/tests/check_psx_frames.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_unreachable.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_psx_scratch.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_psx_ram_load.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_psx_codegen.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_smul_high.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_loop_optimize.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_mips_t9.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_mips_static_frame_alias.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_generic_scheduler.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_local_register_hint.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_nonfaulting_sh_return.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_call_setup_ties.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_private_frame_sw_jump.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_delay_slot_target_first.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_continuation_constants.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}
RUN sh /work/tests/check_spill_home.sh /work/gcc-${VERSION}/gcc /work/gcc-${VERSION}

FROM scratch AS export
COPY prebuilt/linux-i386/2.8.1/ /
COPY --from=build /work/gcc-2.8.1/cc1 /cc1
