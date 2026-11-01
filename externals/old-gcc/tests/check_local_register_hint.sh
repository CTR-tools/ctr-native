#!/bin/sh
# Finite C-frontend/output controls, not execution or a hard-register ABI.
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "LOCAL register-hint smoke outputs: $task_out"

task_compile()
{
    task_lane=$1; task_bin=$2; shift 2
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G0 \
        -msoft-float -fno-loop-optimize -fempty-asm-coalesce -da "$@" \
        -o raw.s "$task_tests/local_register_hint.c") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
    test ! -s "$task_out/$task_lane/stdout"
    test ! -s "$task_out/$task_lane/stderr"
}
task_compile base "$task_baseline" '-DHINT(name)='
task_compile off "$task_prefix" '-DHINT(name)='
task_compile on "$task_prefix"
cmp "$task_out/base/raw.s" "$task_out/off/raw.s"

# Observe actual LOCAL homes, inline-declaration survival and coalesced hints.
awk '
    /^;; Function / { fn=$3 }
    /^;; Register [0-9]+ local hint / {
        p=$3; hint[fn,p]=$6+0; count[fn]++
    }
    /^;; Register [0-9]+ in / { home[fn,$3]=$5+0 }
    fn=="coalesced_conflict" && /addsi3_internal/ { arithmetic=1 }
    fn=="coalesced_conflict" && /REG_DEAD/ { dying=1 }
    END {
        if (count["hinted_copy"]!=2 || count["unavailable_hint"]!=1 || count["hinted_pointer"]!=1 ||
            count["coalesced_conflict"]!=2 || count["inline_twice"]!=2 ||
            count["stronger_return_preference"]!=1 || count["plain_after_hints"] ||
            count["hard_register_unchanged"] || count["optimized_away"] || !arithmetic || !dying) exit 1
        for (key in hint) {
            split(key,parts,SUBSEP); f=parts[1]; expected=hint[key]; actual=home[key]
            if ((f=="hinted_copy" || f=="hinted_pointer") && expected!=actual) exit 1
            if (f=="unavailable_hint" && expected==actual) exit 1
            if ((f=="coalesced_conflict" || f=="stronger_return_preference") && actual!=2) exit 1
        }
    }
' "$task_out/on/local_register_hint.c.lreg"

# The four-scratch alignment-one copy reuses the locals physical10/11 only in
# a disjoint block. Check that reload reserves them AND retains their homes.
awk '
    /^;; Function / { fn=$3 }
    FILENAME==ARGV[1] && fn=="hinted_copy" && /local hint/ { wanted[$3]=$6+0 }
    FILENAME==ARGV[2] && fn=="hinted_copy" {
        if (/Spilling reg 10\./) ten=1
        if (/Spilling reg 11\./) eleven=1
        if (/movstrsi_internal/) copy=1
        for (i=1;i<=NF-2;i++) if ($i in wanted && $(i+1)=="in" && $(i+2)+0==wanted[$i]) retained[$i]=1
    }
    END { for (p in wanted) if (!retained[p]) exit 1; exit !(ten && eleven && copy) }
' "$task_out/on/local_register_hint.c.lreg" "$task_out/on/local_register_hint.c.greg"
for task_instruction in lwl lwr swl swr; do
    grep -Eq "^[[:space:]]*$task_instruction[[:space:]]" "$task_out/on/raw.s"
done
for task_function in hard_register_unchanged plain_after_hints; do
    for task_lane in off on; do
        awk -v name="$task_function" '$0==name ":" { body=1 } body { print }
            body && $1==".end" && $2==name { exit }' \
            "$task_out/$task_lane/raw.s" > "$task_out/$task_lane/$task_function.s"
    done
    cmp "$task_out/off/$task_function.s" "$task_out/on/$task_function.s"
done

for task_case in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    if "$task_gcc" -B"$task_prefix/" -S -O2 -G0 -msoft-float \
        -DHINT_CASE="$task_case" -o "$task_out/invalid-$task_case.s" \
        "$task_tests/local_register_hint.c" > "$task_out/invalid-$task_case.stdout" \
        2> "$task_out/invalid-$task_case.stderr"; then
        exit 1
    fi
    grep -q local_register_hint "$task_out/invalid-$task_case.stderr"
    if grep -Eq 'Internal compiler error|Segmentation fault' "$task_out/invalid-$task_case.stderr"; then exit 1; fi
done
echo 'LOCAL-hint controls passed (inline lifetime, fallback/conflict, ABI/hard-asm, OFF identity, invalid forms, disjoint BLK reuse).'
