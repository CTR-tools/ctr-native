#!/bin/sh
# Finite final-code/RTL controls, not MIPS execution or a performance proof.
set -eu
task_gcc=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd)/$(basename -- "$1")
task_new=$(CDPATH= cd -- "$2" && pwd)
task_baseline=$2
test "$#" -lt 3 || task_baseline=$3
task_old=$(CDPATH= cd -- "$task_baseline" && pwd)
task_here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Continuation-constant controls: $task_out"
test "$task_old" != "$task_new" || echo 'Old==new: historical identity is not qualified.'
task_hold()
{
    sha256sum "$task_here/check_continuation_constants.sh" \
        "$task_here/continuation_constants.c" "$task_here/continuation_shared_tails.c"
    sha256sum "$task_gcc" "$task_new/cc1" "$task_new/cpp" \
        "$task_old/gcc" "$task_old/cc1" "$task_old/cpp"
}
task_hold > "$task_out/BEFORE.sha256"
trap 'task_status=$?; task_hold > "$task_out/AFTER.sha256"; cmp "$task_out/BEFORE.sha256" "$task_out/AFTER.sha256" || task_status=1; printf "exit=%s\n" "$task_status" > "$task_out/RESULT.txt"; exit "$task_status"' 0
task_compile()
{
    task_source=$1; task_lane=$2; task_prefix=$3; shift 3
    task_driver=$task_gcc
    test "$task_prefix" = "$task_new" || task_driver=$task_old/gcc
    mkdir -p "$task_out/$task_source/$task_lane"
    (cd "$task_out/$task_source/$task_lane" && "$task_driver" -B"$task_prefix/" \
        -S -O2 -G0 -msoft-float -da "$@" -o raw.s "$task_here/$task_source.c") \
        > "$task_out/$task_source/$task_lane/stdout" \
        2> "$task_out/$task_source/$task_lane/stderr"
    test ! -s "$task_out/$task_source/$task_lane/stdout"
    test ! -s "$task_out/$task_source/$task_lane/stderr"
}
task_body()
{
    awk -v name="$3" '$0 == name ":" { active=1; found=1 }
        active { print } active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }' "$task_out/$1/$2/raw.s"
}
task_alpha()
{
    # Bijective local-label definition/reference rename, preserving the CFG.
    awk '{ lines[++n]=$0
        if ($0 ~ /^\$L[0-9]+:/) { name=$0; sub(/:.*/,"",name); labels[name]="CANON" ++count } }
        END { for (i=1;i<=n;i++) { text=lines[i]; result=""
            while (match(text,/\$L[0-9]+/)) {
                name=substr(text,RSTART,RLENGTH); if (!(name in labels)) exit 1
                result=result substr(text,1,RSTART-1) labels[name]
                text=substr(text,RSTART+RLENGTH)
            } print result text } }'
}
task_rtl_metadata()
{
    # This one field is the assembler-label global counter, not an RTL UID.
    awk '$1 == "(code_label" { $5="LABEL_NUMBER" } { print }' "$1"
}
task_rtl_body()
{
    awk -v name="$1" '/^;; Function / { active=($3 == name); if (active) found=1 }
        active { if ($1 == "(code_label") $5="LABEL_NUMBER"; print }
        END { exit !found }' "$task_out/continuation_constants/$2/continuation_constants.c.sched2"
}
task_compare_body()
{
    task_body "$1" off "$2" | task_alpha > "$task_out/expected"
    task_body "$1" on "$2" | task_alpha > "$task_out/actual"
    cmp "$task_out/expected" "$task_out/actual"
}
task_slot_seven()
{
    awk '$1 == ".set" && $2 == "noreorder" { mode=1 }
        $1 == ".set" && $2 == "reorder" { mode=0 }
        $1 == "j" && $2 ~ /^\$L[0-9]+$/ { pending=1; next }
        $1 ~ /^[a-z][a-z0-9]*$/ {
            if (pending && mode && $1 == "li" && $2 ~ /,7$/) ++slots
            pending=0
        } END { exit !(slots > 0) }'
}
for task_source in continuation_constants continuation_shared_tails; do
    task_compile "$task_source" old "$task_old"
    task_compile "$task_source" default "$task_new"
    task_compile "$task_source" off "$task_new" -fno-sched-continuation-constants
    task_compile "$task_source" on "$task_new" -fsched-continuation-constants
    task_compile "$task_source" last-off "$task_new" -fsched-continuation-constants -fno-sched-continuation-constants
    task_compile "$task_source" last-on "$task_new" -fno-sched-continuation-constants -fsched-continuation-constants
    task_compile "$task_source" disabled-off "$task_new" -fno-schedule-insns2 -fno-sched-continuation-constants
    task_compile "$task_source" disabled-on "$task_new" -fno-schedule-insns2 -fsched-continuation-constants
    for task_lane in default off last-off; do
        cmp "$task_out/$task_source/old/raw.s" "$task_out/$task_source/$task_lane/raw.s"
        cmp "$task_out/$task_source/old/$task_source.c.sched2" "$task_out/$task_source/$task_lane/$task_source.c.sched2"
    done
    for task_suffix in sched greg; do
        task_rtl_metadata "$task_out/$task_source/old/$task_source.c.$task_suffix" > "$task_out/expected"
        for task_lane in default off on last-off last-on; do
            task_rtl_metadata "$task_out/$task_source/$task_lane/$task_source.c.$task_suffix" > "$task_out/actual"
            cmp "$task_out/expected" "$task_out/actual"
        done
    done
    cmp "$task_out/$task_source/on/raw.s" "$task_out/$task_source/last-on/raw.s"
    cmp "$task_out/$task_source/on/$task_source.c.sched2" "$task_out/$task_source/last-on/$task_source.c.sched2"
    cmp "$task_out/$task_source/disabled-off/raw.s" "$task_out/$task_source/disabled-on/raw.s"
done
task_compare_body continuation_constants PublishAndScan
task_compare_body continuation_shared_tails OrdinarySharedTail
for task_name in TargetWriteFirst SourceOverwriteFirst TargetCall TargetControlSplit \
    ConditionalEnd LoopControl ExpensiveMaterialization; do
    task_compare_body continuation_constants "$task_name"
    task_rtl_body "$task_name" off > "$task_out/expected"
    task_rtl_body "$task_name" on > "$task_out/actual"
    cmp "$task_out/expected" "$task_out/actual"
done
for task_pair in continuation_constants:ReportAndRestart continuation_shared_tails:OtherJumpContinuation; do
    task_source=$(printf '%s\n' "$task_pair" | cut -d: -f1)
    task_name=$(printf '%s\n' "$task_pair" | cut -d: -f2)
    task_body "$task_source" off "$task_name" | task_alpha > "$task_out/expected"
    task_body "$task_source" on "$task_name" | task_alpha > "$task_out/actual"
    if cmp -s "$task_out/expected" "$task_out/actual"; then
        echo "missing real final-code change: $task_name" >&2; exit 1
    fi
    task_body "$task_source" on "$task_name" | task_slot_seven
done
task_before=$(task_body continuation_shared_tails off OtherJumpContinuation | awk '$1 == "sw" { n++ } END { print n+0 }')
task_after=$(task_body continuation_shared_tails on OtherJumpContinuation | awk '$1 == "sw" { n++ } END { print n+0 }')
test "$task_after" -eq "$((task_before + 2))"
echo 'Continuation-constant controls passed: real constant slots, retained two-jump stores, ordinary sharing, default/inactive/refusal controls.'
echo 'Expensive materialization reaches overwrite refusal; loop control reaches the direct-tail gate, not a special loop predicate.'
