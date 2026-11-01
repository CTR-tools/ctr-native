#!/bin/sh
set -eu
task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Nonfaulting SH return smoke outputs: $task_out"

task_compile()
{
    task_lane=$1; shift
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_prefix/" -S -O2 -G0 \
        -msoft-float -mno-split-addresses -fno-schedule-insns -fno-schedule-insns2 \
        -da "$@" "$task_tests/nonfaulting_sh_return.c" -o raw.s) \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_ok()
{
    task_compile "$@"
    test ! -s "$task_out/$task_lane/stdout"
    test ! -s "$task_out/$task_lane/stderr"
}
task_body()
{
    awk -v name="$1" '$0 == name ":" { active=1; found=1 }
        active && $1 !~ /^#/ { print }
        active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }' "$task_out/$2/raw.s"
}
task_sh_return()
{
    awk '$1 == "j" && $2 == "$31" { ret=1; next }
        ret && ($1 ~ /^#/ || $1 == ".set") { next }
        ret && $1 == "sh" { hit=1; ret=0; next }
        ret && NF { ret=0 }
        END { exit !hit }'
}
task_check_shapes()
{
    task_off=$1; task_on=$2
    for task_name in StoreLow StoreBare StoreZero StoreTwo StoreNegative; do
        task_body "$task_name" "$task_on" | task_sh_return
        if task_body "$task_name" "$task_off" | task_sh_return; then exit 1; fi
    done
    for task_name in ByteStore WordStore PlainStore SymbolStore OddStore \
        StoreThenBarrier StoreBeforeCall FramedStore ConditionalStore ReturnAddressBase StackBase; do
        task_body "$task_name" "$task_off" > "$task_out/before.s"
        task_body "$task_name" "$task_on" > "$task_out/after.s"
        cmp "$task_out/before.s" "$task_out/after.s"
    done
    task_body StoreTwo "$task_on" | awk '
        $1 == "sh" { if (++stores == 1) first=NR; else second=NR }
        $1 == "j" && $2 == "$31" { ret=NR }
        END { exit !(stores == 2 && first < ret && ret < second) }'
}

task_ok default
task_ok off -mnonfaulting-sh-return=off
cmp "$task_out/default/raw.s" "$task_out/off/raw.s"
task_ok on -mnonfaulting-sh-return=on
task_check_shapes off on
task_body StoreThenAlu off > "$task_out/before.s"
task_body StoreThenAlu on > "$task_out/after.s"
cmp "$task_out/before.s" "$task_out/after.s"
task_ok last-off -mnonfaulting-sh-return=invalid -mnonfaulting-sh-return=on -mnonfaulting-sh-return=off
task_ok last-on -mnonfaulting-sh-return=off -mnonfaulting-sh-return=on
cmp "$task_out/off/raw.s" "$task_out/last-off/raw.s"
cmp "$task_out/on/raw.s" "$task_out/last-on/raw.s"
task_ok ordinary-off -msplit-addresses -fschedule-insns -fschedule-insns2 -mnonfaulting-sh-return=off
task_ok ordinary-on -msplit-addresses -fschedule-insns -fschedule-insns2 -mnonfaulting-sh-return=on
task_check_shapes ordinary-off ordinary-on

task_profile()
{
    task_profile_name=$1; shift
    task_off_status=0; task_on_status=0
    task_compile "$task_profile_name-off" "$@" -mnonfaulting-sh-return=off || task_off_status=$?
    task_compile "$task_profile_name-on" "$@" -mnonfaulting-sh-return=on || task_on_status=$?
    test "$task_off_status" -eq "$task_on_status"
    cmp "$task_out/$task_profile_name-off/stderr" "$task_out/$task_profile_name-on/stderr"
    if test "$task_off_status" -eq 0; then
        cmp "$task_out/$task_profile_name-off/raw.s" "$task_out/$task_profile_name-on/raw.s"
    fi
    if grep -E 'internal compiler error|fatal signal|Segmentation' "$task_out/$task_profile_name-on/stderr"; then exit 1; fi
}
task_profile mips2 -mips2 -mcpu=r4000
task_profile big-endian -EB
task_profile hard-float -mhard-float
task_profile pic -fPIC
task_profile abicalls -mabicalls
task_profile long-calls -mlong-calls
task_profile embedded-pic -membedded-pic
task_profile abi64 -mips3 -mcpu=r4000 -mabi=64
task_profile gp64 -mips3 -mcpu=r4000 -mgp64
if task_compile invalid -mnonfaulting-sh-return=invalid; then exit 1; fi
grep -q 'bad value' "$task_out/invalid/stderr"
if grep -E 'internal compiler error|fatal signal|Segmentation' "$task_out/invalid/stderr"; then exit 1; fi
