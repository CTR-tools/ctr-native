#!/bin/sh
# Finite compiler-output controls; no MIPS execution or hardware timing claim.
set -eu
task_gcc=$1
task_prefix=$2
task_old=${3:-$task_prefix}
# Optional third prefix supplies a retained compiler; otherwise base is current.
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Call-setup tie outputs: $task_out"
task_pin()
{
    sha256sum "$0" "$task_tests/call_setup_ties.c"
    for task_dir in "$task_prefix" "$task_old"; do
        for task_name in cc1 cc1plus cpp g++ gcc; do
            sha256sum "$task_dir/$task_name"
        done
    done
}
task_pin > "$task_out/before.sha256"
task_compile()
{
    task_lane=$1; task_bin=$2; shift 2
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G0 \
        -msoft-float -da "$@" -o raw.s "$task_tests/call_setup_ties.c") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
    test ! -s "$task_out/$task_lane/stdout"
    test ! -s "$task_out/$task_lane/stderr"
}
task_compile base "$task_old"
task_compile default "$task_prefix"
task_compile off "$task_prefix" -fno-sched2-late-inplace-at-call-setup
task_compile on "$task_prefix" -fsched2-late-inplace-at-call-setup
task_compile last-off "$task_prefix" -fsched2-late-inplace-at-call-setup -fno-sched2-late-inplace-at-call-setup
task_compile last-on "$task_prefix" -fno-sched2-late-inplace-at-call-setup -fsched2-late-inplace-at-call-setup
for task_lane in default off last-off; do
    cmp "$task_out/base/raw.s" "$task_out/$task_lane/raw.s"
done
cmp "$task_out/on/raw.s" "$task_out/last-on/raw.s"
cmp "$task_out/on/call_setup_ties.c.sched2" "$task_out/last-on/call_setup_ties.c.sched2"
for task_pass in rtl jump addressof cse loop cse2 bp flow combine sched lreg greg; do
    for task_lane in default off on last-off last-on; do
        cmp "$task_out/base/call_setup_ties.c.$task_pass" "$task_out/$task_lane/call_setup_ties.c.$task_pass"
    done
done
for task_fn in NoCall ArgumentUpdate MemoryInput AsmUpdate DivideUpdate DependentArgument; do
    for task_lane in off on; do
        awk -v name="$task_fn" '
            $1==".ent" && $2==name {active=1}
            active {print}
            $1==".end" && $2==name {active=0}
        ' "$task_out/$task_lane/raw.s" > "$task_out/$task_lane/$task_fn.s"
        test -s "$task_out/$task_lane/$task_fn.s"
    done
    cmp "$task_out/off/$task_fn.s" "$task_out/on/$task_fn.s"
done
awk '
function hex(s, v,i) {gsub(/[()]/,"",s);v=0;for(i=1;i<=length(s);i++)v=16*v+index("0123456789abcdef",substr(s,i,1))-1;return v}
function flushready( k,max) {
    if(!chosen)return
    max=-1
    for(k in eventpri)if(!blocked[k] && eventpri[k]>max)max=eventpri[k]
    if(!(chosen in eventpri) || blocked[chosen] || eventpri[chosen]<max)bad=1
    events++
    for(k in eventpri)delete eventpri[k]
    for(k in blocked)delete blocked[k]
    chosen=0
}
function regdef(s, t) {
    if (!match(s, /\(set[[:space:]]+\(reg[^ :]*:[A-Z]+ [0-9]+/)) return -1
    t=substr(s,RSTART,RLENGTH); sub(/^.*:[A-Z]+ /,"",t); return t+0
}
function callarg(file,f,u, c,r,s) {
    r=regdef(record[file,f,u]); if(r<0)return 0
    for(c in calls) {
        split(c,parts,SUBSEP)
        if(parts[1]!=file || parts[2]!=f)continue
        s=record[file,f,parts[3]]
        if(s ~ ("\\(insn_list " u "[ )]") &&
           s ~ ("\\(use \\(reg[^ :]*:[A-Z]+ " r "[ )]"))return 1
    }
    return 0
}
function checkdeps(file,f,u, text,part,p) {
    text=record[file,f,u]
    while(match(text,/\(insn_list [0-9]+[ )]/)) {
        part=substr(text,RSTART,RLENGTH);sub(/^.*list /,"",part);p=part+0
        if((file SUBSEP f SUBSEP p) in position && p!=u) {
            edges++
            if(position[file,f,p]>=position[file,f,u])bad=1
        }
        text=substr(text,RSTART+RLENGTH)
    }
}
/^;; Function / { flushready();fn=$3; names[FILENAME,fn]=1; node=0 }
/^;; ready list at T-/ {
    flushready();ready=$0;sub(/, now.*/,"",ready)
    text=ready;sub(/^.*: /,"",text);count=split(text,items," ")
    for(j=1;j<=count;j+=2)eventpri[items[j]+0]=hex(items[j+1])
    text=$0;sub(/^.*, now /,"",text);chosen=text+0
}
/blocking insn/ {blocked[$4+0]=1;blocks++}
/with [1-9][0-9]* stalls/ {stalls++}
/has a greater potential hazard/ {chosen=$3+0}
/^;; total time =/ {flushready()}
/is a tied pure update at actual call setup/ {
    u=$3+0;chosen=u; witnesses[FILENAME,fn]++; witness[FILENAME,fn]=u
    witness_ready[FILENAME,fn]=ready
}
/^\((insn|jump_insn|call_insn)[ /]/ {
    node=$2+0; record[FILENAME,fn,node]=$0
    position[FILENAME,fn,node]=++order[FILENAME,fn]
    if($0 ~ /^\(call_insn/)calls[FILENAME,fn,node]=1
    next
}
node && NF { record[FILENAME,fn,node]=record[FILENAME,fn,node] "\n" $0; next }
!NF { node=0 }
END {
    off=ARGV[1]; on=ARGV[2]
    n=split("UpdateOr UpdateAdd NoCall ArgumentUpdate MemoryInput AsmUpdate DivideUpdate DependentArgument", functions," ")
    for(i=1;i<=n;i++) {
        f=functions[i]
        if(!names[off,f] || !names[on,f] || witnesses[off,f])bad=1
        if(f=="UpdateOr" || f=="UpdateAdd") {
            if(witnesses[on,f]!=1) {bad=1;continue}
            u=witness[on,f]; s=record[on,f,u]; r=regdef(s)
            if(r<0 || callarg(on,f,u))bad=1
            if(f=="UpdateOr" && s!~/\(ior:SI/)bad=1
            if(f=="UpdateAdd" && s!~/\(plus:SI/)bad=1
            if(s!~("\\((ior|plus):SI \\(reg[^ :]*:SI " r "[ )]"))bad=1
            ready=witness_ready[on,f]; sub(/^.*: /,"",ready)
            count=split(ready,items," "); args=0; pri=""
            for(j=1;j<=count;j+=2) {
                v=items[j]+0; q=items[j+1]
                if(pri!="" && pri!=q)bad=1
                pri=q
                if(v!=u && callarg(on,f,v))args++
            }
            if(!args)bad=1
        } else if(witnesses[on,f])bad=1
    }
    for(k in record) {
        split(k,parts,SUBSEP)
        checkdeps(parts[1],parts[2],parts[3])
    }
    if(edges<30 || events<100 || stalls<2)bad=1
    # The retained arg update must actually read its own destination and feed CALL.
    found=0
    for(k in record) {
        split(k,parts,SUBSEP)
        if(parts[1]!=on || parts[2]!="ArgumentUpdate")continue
        s=record[k];r=regdef(s)
        if(r>=0 && s~/\(ior:SI/ &&
           s~("\\(ior:SI \\(reg[^ :]*:SI " r "[ )]") && callarg(on,parts[2],parts[3]))found++
    }
    if(found!=1)bad=1
    if(bad) {print "Call-setup RTL assertions failed";exit 1}
    print "Independent RTL controls passed: two arithmetic ties; actual CALL USE roles; dual-role exclusion; " edges " preserved TRUE edges."
}' "$task_out/off/call_setup_ties.c.sched2" "$task_out/on/call_setup_ties.c.sched2"
task_pin > "$task_out/after.sha256"
cmp "$task_out/before.sha256" "$task_out/after.sha256"
echo 'Default/OFF/base/precedence and all pre-sched2 passes agree; frozen inputs held.'
