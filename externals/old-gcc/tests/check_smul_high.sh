#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Signed-high value smoke outputs: $task_out"

task_body()
{
    awk -v name="$1" '$0 == name ":" { active=1; found=1 }
        active { print }
        active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }' "$2"
}
task_count()
{
    task_body "$1" "$task_lane/raw.s" > "$task_out/body.s"
    awk -v op="$2" -v want="$3" '
        $1 == op { count++ } END { exit (count != want) }' "$task_out/body.s"
}

for task_level in 0 2; do
    task_lane="$task_out/O$task_level"
    mkdir "$task_lane"
    (cd "$task_lane" && "$task_gcc" -B"$task_prefix/" -S "-O$task_level" \
        -G0 -msoft-float -ffunction-sections -da -o raw.s "$task_tests/smul_high.c") \
        > "$task_lane/stdout" 2> "$task_lane/stderr"
    test ! -s "$task_lane/stdout"; test ! -s "$task_lane/stderr"
    (cd "$task_lane" && "$task_gcc" -B"$task_prefix/" -S "-O$task_level" \
        -G0 -msoft-float -ffunction-sections -o provider.s "$task_tests/smul_high_provider.c") \
        > "$task_lane/provider.stdout" 2> "$task_lane/provider.stderr"
    test ! -s "$task_lane/provider.stdout"; test ! -s "$task_lane/provider.stderr"

    task_count HighDynamic mult 1
    task_count HighDynamic mfhi 1
    task_count HighConstant mult 1
    task_count HighConstant mfhi 1
    task_count HighAcrossHigh mult 2
    task_count HighAcrossHigh mfhi 2
    task_count HighAcrossLow mult 2
    task_count HighAcrossLow mflo 1
    task_count HighAcrossCall jal 1
    task_count HighAcrossCall mfhi 1
    task_count HighCallArguments jal 2
    task_count HighCallArguments mfhi 1
    task_count HighUpdates mfhi 1
    task_count HighConverted mfhi 1
    for task_name in HighDynamic HighConstant HighZero HighAcrossHigh \
        HighAcrossCall HighUpdates HighIgnoredUpdates HighCallArguments HighConverted; do
        task_count "$task_name" mflo 0
    done

    # Inspect actual pre-update value copies, not chosen pseudo numbers.
    awk 'BEGIN { RS="" }
        function reg(s, t) {
            if (!match(s, /\(reg[^ :]*:SI [0-9]+/)) return ""
            reg_end=RSTART+RLENGTH
            t=substr(s,RSTART,RLENGTH); sub(/^.* /,"",t); return t
        }
        /;; Function / { active=($3 == "HighUpdates") }
        active && /^\(insn / {
            order++
            if (/\(mult:DI /) {
                text=$0
                sub(/^.*\(mult:DI /,"",text)
                a=reg(text); text=substr(text,reg_end)
                b=reg(text)
                if (!a || !b || a == b || !copy[a] || !copy[b] ||
                    copy[a] == copy[b] || !inc[copy[a]] || !inc[copy[b]] ||
                    born[a] >= inc[copy[a]] || born[b] >= inc[copy[b]] ||
                    inc[copy[a]] >= order || inc[copy[b]] >= order) bad=1
                products++
            } else if (/\(set \(reg[^ :]*:SI /) {
                text=$0; sub(/^.*\(set /,"",text); dst=reg(text)
                sub(/^\(reg[^)]*\)/,"",text); src=reg(text)
                if (/\(plus:SI / && /\(const_int 1\)/ && dst == src) inc[dst]=order
                else if (dst && src && dst != src && !/\(plus:/) {
                    copy[dst]=src; born[dst]=order
                }
            }
        }
        END { exit !(products == 1 && !bad) }
    ' "$task_lane/smul_high.c.rtl"
    awk 'BEGIN { RS="" } /\{smulsi3_highpart\}/ {
        if (!/\(clobber \(reg:SI 65 lo\)\)/ ||
            !/\(clobber \(reg:SI 66 accum\)\)/) bad=1
        found++
    } END { exit !(found && !bad) }' "$task_lane/smul_high.c.greg"
done

task_lane="$task_out/O2"
task_count HighZero mult 0
task_count HighIgnoredUpdates mult 0
task_count HighIgnoredUpdates sw 2
for task_name in HighAcrossHigh HighAcrossLow; do
    task_read=mfhi
    if test "$task_name" = HighAcrossLow; then task_read=mflo; fi
    task_body "$task_name" "$task_lane/raw.s" | awk -v readop="$task_read" '
        $1 == "mult" { writers++; if (writers == 2) {
            split($2,args,","); if (args[1] != first && args[2] != first) bad=1
        } }
        $1 == "mfhi" && !first { first=$2; next }
        $1 == readop && writers == 2 { second=$2 }
        $1 == "xor" { split($2,args,",");
            if (!((args[2] == first && args[3] == second) ||
                  (args[3] == first && args[2] == second))) bad=1
            uses++
        }
        END { exit !(writers == 2 && first && second && first != second && uses == 1 && !bad) }
    '
done

for task_arity in 0 1 3; do
    if "$task_gcc" -B"$task_prefix/" -S -O2 -DARITY="$task_arity" \
        -o "$task_out/invalid-$task_arity.s" "$task_tests/smul_high.c" \
        > "$task_out/invalid-$task_arity.stdout" 2> "$task_out/invalid-$task_arity.stderr"; then
        exit 1
    else
        task_status=$?
    fi
    test "$task_status" -eq 1
    task_message='too few arguments'
    if test "$task_arity" -eq 3; then task_message='too many arguments'; fi
    grep -Fq "$task_message to function " "$task_out/invalid-$task_arity.stderr"
    grep -Fq '__builtin_smul_high_si' "$task_out/invalid-$task_arity.stderr"
    if grep -Eiq 'internal compiler error|segmentation fault|fatal signal' \
        "$task_out/invalid-$task_arity.stderr"; then exit 1; fi
done
