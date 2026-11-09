#!/bin/sh
# Finite compiler-output controls, not an original-source or MIPS execution proof.
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_native_cc=${4:-}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "spill-home smoke outputs: $task_out"

task_compile()
{
    task_lane=$1; task_bin=$2; task_source=$3; shift 3
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G0 \
        -msoft-float -fno-caller-saves -da "$@" -o raw.s "$task_tests/$task_source") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_clean()
{
    test ! -s "$task_out/$1/stdout"
    test ! -s "$task_out/$1/stderr"
}
task_body()
{
    awk -v name="$2" '$0==name ":" { body=1 } body { print }
        body && $1==".end" && $2==name { exit }' "$task_out/$1/raw.s"
}
task_bad()
{
    task_bad_lane=$1; shift
    if task_compile "$task_bad_lane" "$task_prefix" "$@"; then
        echo "expected spill_home rejection: $task_bad_lane" >&2; exit 1
    fi
    grep -q spill_home "$task_out/$task_bad_lane/stderr"
    if grep -Eq 'Internal compiler error|Segmentation fault' \
        "$task_out/$task_bad_lane/stderr"; then exit 1; fi
}

task_compile old "$task_baseline" spill_home.c '-DHOME(n)='
task_compile off "$task_prefix" spill_home.c '-DHOME(n)='
task_compile on "$task_prefix" spill_home.c
task_clean old; task_clean off; task_clean on
cmp "$task_out/old/raw.s" "$task_out/off/raw.s"
task_body on real_snapshots > "$task_out/si.s"
task_body on real_short_snapshots > "$task_out/hi.s"
grep -Eq '\.frame[[:space:]]+\$sp,144,.*vars=[[:space:]]*88' "$task_out/si.s"
grep -Eq '\.frame[[:space:]]+\$sp,192,.*vars=[[:space:]]*136' "$task_out/hi.s"
for task_op in sw lw; do
    grep -Eq '^[[:space:]]*'"$task_op"'[[:space:]]+[^,]+,80\(\$sp\)' "$task_out/si.s"
done
grep -Eq '^[[:space:]]*sh[[:space:]]+[^,]+,112\(\$sp\)' "$task_out/hi.s"
grep -Eq '^[[:space:]]*lhu?[[:space:]]+[^,]+,112\(\$sp\)' "$task_out/hi.s"
grep -Eq '^[[:space:]]*sh[[:space:]]+[^,]+,120\(\$sp\)' "$task_out/hi.s"
for task_function in register_resident eliminated plain_after; do
    task_body off "$task_function" > "$task_out/off-$task_function.s"
    task_body on "$task_function" > "$task_out/on-$task_function.s"
    cmp "$task_out/off-$task_function.s" "$task_out/on-$task_function.s"
    grep -Eq '\.frame[[:space:]]+\$sp,0,' "$task_out/on-$task_function.s"
done
task_compile wide "$task_prefix" spill_home.c -DWIDE_CALL=1
task_clean wide
task_body wide real_snapshots > "$task_out/wide.s"
grep -q 'args= 24' "$task_out/wide.s"
for task_op in sw lw; do
    grep -Eq '^[[:space:]]*'"$task_op"'[[:space:]]+[^,]+,88\(\$sp\)' "$task_out/wide.s"
done
task_bad backward spill_home.c -DOFFSET=0
task_bad overlap spill_home.c '-DSECOND=HOME(64)'
task_bad misaligned-si spill_home.c -DOFFSET=66
task_bad misaligned-hi spill_home.c '-DHOME(n)=__attribute__((spill_home(100)))'
for task_case in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
    task_bad "invalid-$task_case" spill_home_invalid.c "-DCASE=$task_case"
done
# Frontend target refusals use no pre-existing aggregate alignment request.
for task_flag in -mips2 -EB -mstack-align=4 -mhard-float -mabicalls -fpic; do
    task_bad "target$task_flag" spill_home_invalid.c "$task_flag"
done
# These locals actually survive, so their context guards are exercised.
for task_flag in -g -O0 -p -fprofile-arcs -ftest-coverage; do
    task_bad "context$task_flag" spill_home.c "$task_flag"
done
task_compile block-warning "$task_prefix" spill_home.c -a
grep -q "basic block profile.*not supported" "$task_out/block-warning/stderr"
test ! -s "$task_out/block-warning/stdout"
cmp "$task_out/on/raw.s" "$task_out/block-warning/raw.s"

if test -n "$task_native_cc"; then
    "$task_native_cc" -O2 -fsanitize=address,undefined -fno-sanitize-recover=all \
        '-DHOME(n)=' -DNATIVE_CHECK "$task_tests/spill_home.c" -o "$task_out/native"
    "$task_out/native"
fi
echo 'spill-home controls passed (real SI/HI homes, inactive/default identity, origin and checked refusals).'
