#!/bin/sh
# Compiler-output controls only; unsupported addresses are never executed.
set -eu
task_gcc=$1; task_prefix=$2; task_old=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d "${TMPDIR:-/tmp}/private-frame-sw-jump.XXXXXXXX")
echo "Private-frame SW/J controls: $task_out"
task_hold()
{
    sha256sum "$task_tests/check_private_frame_sw_jump.sh" "$task_tests/private_frame_sw_jump.c" \
        "$task_gcc" "$task_prefix/cc1" "$task_prefix/cpp" \
        "$task_old/gcc" "$task_old/cc1" "$task_old/cpp"
}
task_hold > "$task_out/BEFORE.sha256"
trap 'task_status=$?; task_hold > "$task_out/AFTER.sha256"; cmp "$task_out/BEFORE.sha256" "$task_out/AFTER.sha256" || task_status=1; exit "$task_status"' 0
task_compile()
{
    task_lane=$1; shift
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_prefix/" -S -O2 -G0 \
        -msoft-float -mno-split-addresses -da "$@" "$task_tests/private_frame_sw_jump.c" -o raw.s) \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_ok()
{
    task_compile "$@"
    test ! -s "$task_out/$task_lane/stdout"; test ! -s "$task_out/$task_lane/stderr"
}
task_body()
{
    awk -v name="$1" '$0 == name ":" { active=1; found=1 }
        active { print }
        active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }' "$task_out/$2/raw.s"
}
task_instructions()
{
    awk '$1 ~ /^[a-z][a-z0-9]*$/ { print $1, $2 }'
}
task_local_slot()
{
    awk '$1 == ".set" && $2 == "noreorder" { mode=1 }
        $1 == ".set" && $2 == "reorder" { mode=0 }
        $1 == "j" && $2 ~ /^\$L/ { branch=1; next }
        $1 ~ /^[a-z][a-z0-9]*$/ {
            if (branch && $1 == "sw" && $2 ~ /^\$[0-9]+,[0-9]+\(\$sp\)$/) {
                ++count; if (!mode) bad=1
            }
            branch=0
        }
        END { exit !(count == 1 && !bad) }'
}
task_rtl_slots()
{
    awk -v name="$1" -v expected="$3" '
        /^;; Function / { active=($3 == name) }
        active && (depth || /^\(/) {
            node=node $0; text=$0; depth+=gsub(/\(/,"(",text)
            text=$0; depth-=gsub(/\)/,")",text)
            if (!depth) {
                if (index(node,"(sequence[") && index(node,"{jump}")) {
                    ++count; if (gsub(/\(set \(mem\/v:SI/,"&",node) != 1 ||
                        !index(node,"{movsi_internal2}")) bad=1
                }
                node=""
            }
        }
        END { exit !(count == expected && !bad) }' "$task_out/$2/private_frame_sw_jump.c.dbr"
}
task_check()
{
    task_off=$1; task_on=$2
    for task_name in Zero Value Offset BeforeDevice; do
        task_body "$task_name" "$task_on" | task_local_slot
        if task_body "$task_name" "$task_off" | task_local_slot; then exit 1; fi
        task_rtl_slots "$task_name" "$task_on" 1
        task_rtl_slots "$task_name" "$task_off" 0
        task_body "$task_name" "$task_off" | task_instructions | awk '
            previous && $1 == "j" && $2 ~ /^\$L/ && previous ~ /^sw / {
                print; print previous; previous=""; ++swaps; next
            }
            { if (previous) print previous; previous=$0 }
            END { if (previous) print previous; exit !(swaps == 1) }' > "$task_out/expected"
        task_body "$task_name" "$task_on" | task_instructions > "$task_out/actual"
        cmp "$task_out/expected" "$task_out/actual"
    done
    for task_name in Half Byte NonStack Exposed Dynamic WithCall Asm Conditional ReturnStore \
        Computed ConditionalEntry SourceStack LargeFrame NegativeAddress OutsideAddress OddAddress; do
        task_body "$task_name" "$task_off" > "$task_out/before"
        task_body "$task_name" "$task_on" > "$task_out/after"
        cmp "$task_out/before" "$task_out/after"
    done
    task_body Offset "$task_on" | grep -q 'sw.*\$0,16(\$sp)'
    task_body BeforeDevice "$task_on" | awk '$1 == "sw" && $2 == "$2,0($4)" { device=NR }
        $1 == "j" && $2 ~ /^\$L/ { jump=NR } END { exit !(device && device < jump) }'
    for task_pair in NegativeAddress:-4 OutsideAddress:128 OddAddress:1; do
        task_name=${task_pair%:*}; task_offset=${task_pair#*:}
        task_body "$task_name" "$task_on" | grep -q 'frame.*\$sp,8,'
        task_body "$task_name" "$task_on" | grep -Fq "sw$(printf '\t')\$0,$task_offset(\$sp)"
    done
}
task_ok default
task_ok off -mprivate-frame-sw-jump=off
task_ok on -mprivate-frame-sw-jump=on
(task_gcc=$task_old/gcc; task_prefix=$task_old; task_ok old)
cmp "$task_out/old/raw.s" "$task_out/default/raw.s"
cmp "$task_out/default/raw.s" "$task_out/off/raw.s"
task_check off on
task_ok last-off -mprivate-frame-sw-jump=bad -mprivate-frame-sw-jump=on -mprivate-frame-sw-jump=off
task_ok last-on -mprivate-frame-sw-jump=off -mprivate-frame-sw-jump=on
cmp "$task_out/last-off/raw.s" "$task_out/off/raw.s"
cmp "$task_out/last-on/raw.s" "$task_out/on/raw.s"
task_ok no-sched-off -fno-schedule-insns -fno-schedule-insns2 -mprivate-frame-sw-jump=off
task_ok no-sched-on -fno-schedule-insns -fno-schedule-insns2 -mprivate-frame-sw-jump=on
task_check no-sched-off no-sched-on
task_profile()
{
    task_name=$1; shift; task_a=0; task_b=0
    task_compile "$task_name-off" "$@" -mprivate-frame-sw-jump=off || task_a=$?
    task_compile "$task_name-on" "$@" -mprivate-frame-sw-jump=on || task_b=$?
    test "$task_a" -eq "$task_b"
    cmp "$task_out/$task_name-off/stderr" "$task_out/$task_name-on/stderr"
    if test "$task_a" -eq 0; then cmp "$task_out/$task_name-off/raw.s" "$task_out/$task_name-on/raw.s"; fi
    if grep -Ei 'internal compiler error|fatal signal|Segmentation' "$task_out/$task_name-on/stderr"; then exit 1; fi
}
task_profile mips2 -mips2 -mcpu=r4000
task_profile big -EB
task_profile hard -mhard-float
task_profile pic -fPIC
task_profile abicalls -mabicalls
task_profile long -mlong-calls
task_profile embedded -membedded-pic
task_profile abi64 -mips3 -mcpu=r4000 -mabi=64
task_profile gp64 -mips3 -mcpu=r4000 -mgp64
task_profile bgez -mpsx-local-bgez=on
task_profile profile -p
task_profile stack -fstack-check
task_profile frame-pointer -fno-omit-frame-pointer
task_profile exceptions -fexceptions
task_profile manual -DMANUAL
grep -q 'unsupported manual AT ABI mode or stack-storage requirement' "$task_out/manual-on/stderr"
if task_compile invalid -mprivate-frame-sw-jump=invalid; then exit 1; fi
grep -q 'bad value' "$task_out/invalid/stderr"
echo "Private-frame SW/J controls passed"
