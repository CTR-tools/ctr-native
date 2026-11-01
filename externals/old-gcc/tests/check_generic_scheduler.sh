#!/bin/sh
# Finite compiler-clock controls, not hardware timing or general ASM safety.
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
task_source=empty_asm_schedule.c
echo "Generic scheduling smoke outputs: $task_out"
task_compile()
{
    task_lane=$1; task_bin=$2; shift 2
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G0 \
        -msoft-float -da "$@" -o raw.s "$task_tests/$task_source") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
    test ! -s "$task_out/$task_lane/stdout"; test ! -s "$task_out/$task_lane/stderr"
}
task_compile base "$task_baseline"
task_compile default "$task_prefix"
task_compile off "$task_prefix" -fno-empty-asm-coalesce -fno-sched-first-safe-pre-reload -fno-sched-zero-empty-issue-after-reload
task_compile on "$task_prefix" -fsched-zero-empty-issue-after-reload
task_compile last-off "$task_prefix" -fsched-zero-empty-issue-after-reload -fno-sched-zero-empty-issue-after-reload
task_compile last-on "$task_prefix" -fno-sched-zero-empty-issue-after-reload -fsched-zero-empty-issue-after-reload
for task_lane in default off last-off; do cmp "$task_out/base/raw.s" "$task_out/$task_lane/raw.s"; done
cmp "$task_out/on/raw.s" "$task_out/last-on/raw.s"
for task_lane in default off on last-off last-on; do
    cmp "$task_out/base/empty_asm_schedule.c.greg" "$task_out/$task_lane/empty_asm_schedule.c.greg"
done
cmp "$task_out/on/empty_asm_schedule.c.sched2" "$task_out/last-on/empty_asm_schedule.c.sched2"
for task_mode in off on; do
    awk -v enabled="$task_mode" 'BEGIN { RS="" }
        function firstreg(s) {
            if (!match(s,/\(set[[:space:]]+\(reg[^ :]*:SI [0-9]+/)) return -1
            s=substr(s,RSTART,RLENGTH); sub(/^.*:SI /,"",s); return s+0
        }
        function flush() {
            if (chosen) { indexof[fn,chosen]=++steps[fn]; clocks[fn,steps[fn]]=clock; count[fn,chosen]++ }
            chosen=0
        }
        /;; Function / { fn=$3; if (!(fn in names)) found++; names[fn]=1 }
        /;; ready list at T-/ {
            n=split($0,lines,"\n")
            for (i=1;i<=n;i++) {
                line=lines[i]
                if (line ~ /^;; ready list at T-/) {
                    flush(); sub(/^;; ready list at T-/,"",line); sub(/:.*/,"",line); clock=line+0
                }
                line=lines[i]
                if (line ~ /, now [0-9]/) { sub(/^.*, now /,"",line); sub(/ .*/,"",line); chosen=line+0 }
                if (line ~ /^;; total time =/) flush()
            }
        }
        /^\(insn/ {
            id=$2; record[fn,id]=$0
            if (/asm_operands \(""\)/) {
                observations[fn]++; observation[fn]=id
                text=$0; inputs=0
                while (match(text,/\(insn_list [0-9]+ /)) {
                    part=substr(text,RSTART,RLENGTH); sub(/^.*list /,"",part)
                    parent[fn]=part+0; inputs++; text=substr(text,RSTART+RLENGTH)
                }
                if (inputs!=1) bad=1
            }
        }
        END {
            if (found!=3) exit 1
            for (f in names) {
                if (f!="EmptyAlu" && f!="EmptyLoad" && f!="EmptyMultiply") exit 1
                u=observation[f]; p=parent[f]; j=indexof[f,u]
                if (observations[f]!=1 || count[f,u]!=1 || !j || j>=steps[f]) exit 1
                delta=clocks[f,j+1]-clocks[f,j]
                if (firstreg(record[f,p])!=firstreg(record[f,u])) exit 1
                if (f=="EmptyAlu") {
                    if (record[f,p]!~/\{addsi3_internal\}/ || delta!=(enabled=="on"?0:1)) exit 1
                } else if (f=="EmptyLoad") {
                    if (record[f,p]!~/\{movsi_internal2\}/ || record[f,p]!~/\(mem/ || delta<1) exit 1
                } else if (record[f,p]!~/\{mulsi3_internal\}/ || firstreg(record[f,u])!=65 || delta<1) exit 1
            }
            exit bad
        }
    ' "$task_out/$task_mode/empty_asm_schedule.c.sched2"
done
echo 'Empty-ASM controls passed (actual ALU zero-clock; real load/LO overlap refusals; default/OFF/precedence).'

# Keep the inherited policies isolated; these are four more C compiles, not
# a combined option corpus or an instruction/register selector.
task_source=empty_asm_coalesce.c
task_compile coalesce-off "$task_prefix" -ffunction-sections -fno-empty-asm-coalesce
task_compile coalesce-on "$task_prefix" -ffunction-sections -fempty-asm-coalesce
task_source=sched_first_safe.c
task_compile first-safe-off "$task_prefix" -ffunction-sections -fno-sched-first-safe-pre-reload
task_compile first-safe-on "$task_prefix" -ffunction-sections -fsched-first-safe-pre-reload
cmp "$task_out/coalesce-off/empty_asm_coalesce.c.sched" "$task_out/coalesce-on/empty_asm_coalesce.c.sched"
cmp "$task_out/first-safe-off/sched_first_safe.c.combine" "$task_out/first-safe-on/sched_first_safe.c.combine"
awk '# Four READ-ONLY arguments: coalescer OFF/ON raw.s, first-safe OFF/ON .sched.
# Caller first compares coalescer .sched and first-safe .combine literally.
function flush() {
    if (chosen) {
        j=++steps[FILENAME]; selected[FILENAME,j]=chosen
        when[FILENAME,j]=clock; ranked[FILENAME,j]=sorted
        incoming[FILENAME,j]=before; blocked[FILENAME,j]=blocked_now
    }
    chosen=0
}
FILENAME==ARGV[1] || FILENAME==ARGV[2] {
    if ($0=="CoalesceComputed:" || $0=="CoalesceComputedReverse:") {
        body=$0; sub(/:$/,"",body); seen[FILENAME,body]++
    }
    if (body && ($1=="jal" || $1=="jalr")) bad=1
    if (body && $1=="move") moves[FILENAME,body]++
    if (body && $1==".end" && $2==body) { closed[FILENAME,body]++; body="" }
    next
}
FILENAME==ARGV[3] || FILENAME==ARGV[4] {
    if (/^;; Function /) { fn=$3; functions[FILENAME]++ }
    if (/-- basic block number /) {
        active=($0 ~ /-- basic block number 0 /)
    }
    if (active && /^;; ready list at T-/) {
        flush(); before=$0; sub(/^;; ready list at T-[0-9]+: /,"",before)
        sub(/, now.*/,"",before); sorted=""; blocked_now=0
        text=$0; sub(/^;; ready list at T-/,"",text); sub(/:.*/,"",text); clock=text+0
    }
    if (active && /blocking insn/) blocked_now=1
    if (active && /, now [0-9]/) {
        text=$0; sub(/^.*, now /,"",text)
        if (!sorted) sorted=text
        sub(/ .*/,"",text); chosen=text+0
    }
    if (active && /^;; total time =/) flush()
    if (/^\((insn|jump_insn|call_insn)/) {
        node=$2; records[FILENAME,node]=$0
    } else if (node && NF) records[FILENAME,node]=records[FILENAME,node] "\n" $0
    else if (!NF) node=0
    next
}
END {
    off=ARGV[1]; on=ARGV[2]
    n=split("CoalesceComputed CoalesceComputedReverse",names," ")
    for (i=1;i<=n;i++) {
        name=names[i]
        if (seen[off,name]!=1 || seen[on,name]!=1 || closed[off,name]!=1 || closed[on,name]!=1) exit 1
    }
    if (moves[off,"CoalesceComputed"] || moves[on,"CoalesceComputed"] ||
        moves[off,"CoalesceComputedReverse"]!=1 || moves[on,"CoalesceComputedReverse"]) exit 1
    off=ARGV[3]; on=ARGV[4]
    if (functions[off]!=1 || functions[on]!=1 || !steps[off] || steps[off]!=steps[on]) exit 1
    for (i=1;i<=steps[off];i++) {
        if (selected[off,i]!=selected[on,i]) { difference=i; break }
        if (when[off,i]!=when[on,i]) exit 1
    }
    if (!difference || when[off,difference]!=when[on,difference] ||
        incoming[off,difference]!=incoming[on,difference] ||
        ranked[off,difference]!=ranked[on,difference] || blocked[off,difference] || blocked[on,difference]) exit 1
    for (i=2;i<=difference;i++) if (when[off,i]!=when[off,i-1]+1 || when[on,i]!=when[on,i-1]+1) exit 1
    split(ranked[off,difference],ready," "); first=ready[1]+0
    a=selected[off,difference]; b=selected[on,difference]
    if (a==first || b!=first || records[off,a]!~/\{smulsi3_highpart\}/ ||
        records[on,b]!~/\{movsi_internal2\}/ || records[on,b]~/\(mem/ ||
        records[on,b]!~/\(set \(reg:SI [4-7] [^)]*\)[[:space:]]*\(reg:SI [4-7] [^)]*\)\)/) exit 1
    text=incoming[off,difference]; count=0; priority=""
    while (match(text,/\([0-9a-f]+\)/)) {
        value=substr(text,RSTART,RLENGTH)
        if (priority && priority!=value) exit 1
        priority=value; count++; text=substr(text,RSTART+RLENGTH)
    }
    if (count<2 || bad) exit 1
    print "Inherited controls passed: real coalescer copy removal; first-safe equal-height unblocked ready tie."
}
' "$task_out/coalesce-off/raw.s" "$task_out/coalesce-on/raw.s" \
    "$task_out/first-safe-off/sched_first_safe.c.sched" "$task_out/first-safe-on/sched_first_safe.c.sched"
