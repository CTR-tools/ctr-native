#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Compiler smoke outputs: $task_out"

task_extract_function()
{
    awk -v name="$1" '
        $0 == name ":" { body = 1 }
        body { print }
        body && $1 == ".end" && $2 == name { exit }
    ' "$2" > "$3"
}

"$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
    -fno-schedule-insns -fno-schedule-insns2 \
    -o "$task_out/gte.s" "$task_tests/psx_gte_transfers.c"
grep -Eq 'mfc2[[:space:]]+\$2,\$14' "$task_out/gte.s"
grep -Eq 'lwc2[[:space:]]+\$0,4\(\$4\)' "$task_out/gte.s"

for task_mode in default off on; do
    set --
    if [ "$task_mode" != default ]; then
        set -- "-mpsx-normal-ram-lhu-delay=$task_mode"
    fi
    "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
        -fno-schedule-insns -fno-schedule-insns2 "$@" \
        -o "$task_out/lhu-$task_mode.s" "$task_tests/psx_normal_ram_lhu_delay.c"
done
cmp "$task_out/lhu-default.s" "$task_out/lhu-off.s"
sed '/^[[:space:]]*#/d' "$task_out/lhu-on.s" \
    | grep -A1 -E 'beq[[:space:]]+\$13,\$0,' \
    | grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)'
if sed '/^[[:space:]]*#/d' "$task_out/lhu-default.s" \
    | grep -A1 -E 'beq[[:space:]]+\$13,\$0,' \
    | grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)'; then
    exit 1
fi

# Keep ordinary unary minus distinct from the explicit signed-SI operation.
"$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
    -o "$task_out/negation.s" "$task_tests/psx_signed_negation.c"
for task_name in sc_ternary_nonpositive bn_runtime bn_conditional \
    bn_argument_once bn_assignment_overlap; do
    task_extract_function "$task_name" "$task_out/negation.s" "$task_out/$task_name.s"
done
grep -Eq '^[[:space:]]*subu[[:space:]]' "$task_out/sc_ternary_nonpositive.s"
if grep -Eq '^[[:space:]]*sub[[:space:]]' "$task_out/sc_ternary_nonpositive.s"; then
    exit 1
fi
for task_name in bn_runtime bn_conditional bn_argument_once bn_assignment_overlap; do
    grep -Eq '^[[:space:]]*sub[[:space:]]' "$task_out/$task_name.s"
done
for task_name in bn_runtime bn_assignment_overlap; do
    grep -A1 -E '^[[:space:]]*j[[:space:]]+\$31' "$task_out/$task_name.s" \
        | grep -Eq 'sub[[:space:]]+\$2,\$0,\$4'
done
grep -Eq 'beq[[:space:]]+\$5,\$0,' "$task_out/bn_conditional.s"
grep -Eq 'sub[[:space:]]+\$2,\$0,\$2' "$task_out/bn_conditional.s"
test "$(grep -Ec '^[[:space:]]*lw[[:space:]]' "$task_out/bn_argument_once.s")" -eq 1
test "$(grep -Ec '^[[:space:]]*sw[[:space:]]' "$task_out/bn_argument_once.s")" -eq 1
grep -Eq 'addu[[:space:]]+\$3,\$3,1' "$task_out/bn_argument_once.s"
grep -A1 -E '^[[:space:]]*j[[:space:]]+\$31' "$task_out/bn_argument_once.s" \
    | grep -Eq 'sw[[:space:]]+\$3,0\(\$4\)'

# Reject actual use of this MIPS-I-only builtin in an unsupported mode.
"$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -mcpu=r4000 -mips2 \
    -o "$task_out/unsupported-control.s" "$task_tests/section_attribute.c"
if "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -mcpu=r4000 -mips2 \
    -o "$task_out/unsupported.s" "$task_tests/psx_signed_negation.c" \
    > "$task_out/unsupported.stdout" 2> "$task_out/unsupported.stderr"; then
    exit 1
fi
grep -q '__builtin_mips_neg_si requires little-endian MIPS-I' "$task_out/unsupported.stderr"

# The typed control write is ordered even when its return-slot policy is off.
for task_mode in default off on; do
    set --
    if [ "$task_mode" != default ]; then
        set -- "-mpsx-ctc2-return-delay=$task_mode"
    fi
    "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
        "$@" -o "$task_out/ctc2-$task_mode.s" "$task_tests/psx_ctc2.c"
    for task_name in psx_ctc2_write4 psx_ctc2_write31 psx_ctc2_ordered psx_ctc2_postincrement; do
        task_extract_function "$task_name" "$task_out/ctc2-$task_mode.s" \
            "$task_out/$task_name-$task_mode.s"
    done
done
cmp "$task_out/ctc2-default.s" "$task_out/ctc2-off.s"
for task_control in 4 31; do
    for task_mode in default off on; do
        grep -Eq '^[[:space:]]*ctc2[[:space:]]+\$4,\$'"$task_control"'([[:space:]]|$)' \
            "$task_out/psx_ctc2_write$task_control-$task_mode.s"
    done
    grep -A1 -E '^[[:space:]]*j[[:space:]]+\$31' "$task_out/psx_ctc2_write$task_control-on.s" \
        | grep -Eq 'ctc2[[:space:]]+\$4,\$'"$task_control"
    if grep -A1 -E '^[[:space:]]*j[[:space:]]+\$31' "$task_out/psx_ctc2_write$task_control-default.s" \
        | grep -Eq 'ctc2[[:space:]]'; then
        exit 1
    fi
done
for task_mode in default off on; do
    test "$(awk '/^[[:space:]]*ctc2[[:space:]]/ {
        split($2, fields, ","); sub(/^\$/, "", fields[2]); printf "%s", fields[2]
    }' "$task_out/psx_ctc2_ordered-$task_mode.s")" = 01234
    task_body="$task_out/psx_ctc2_postincrement-$task_mode.s"
    test "$(grep -Ec '^[[:space:]]*lw[[:space:]]' "$task_body")" -eq 1
    test "$(grep -Ec '^[[:space:]]*sw[[:space:]]' "$task_body")" -eq 1
    grep -Eq '^[[:space:]]*move[[:space:]]+\$3,\$2' "$task_body"
    grep -Eq '^[[:space:]]*sw[[:space:]]+\$2,0\(\$4\)' "$task_body"
    grep -Eq '^[[:space:]]*ctc2[[:space:]]+\$3,\$4' "$task_body"
    awk '
        /^[[:space:]]*sw[[:space:]]/ { store = NR }
        /^[[:space:]]*ctc2[[:space:]]/ { transfer = NR }
        END { exit !(store && transfer && store < transfer) }
    ' "$task_body"
done

for task_case in VARIABLE_INDEX NEGATIVE_INDEX LARGE_INDEX; do
    if "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float \
        "-DTEST_$task_case=1" -o "$task_out/ctc2-rejected-$task_case.s" "$task_tests/psx_ctc2.c" \
        > "$task_out/ctc2-rejected-$task_case.stdout" 2> "$task_out/ctc2-rejected-$task_case.stderr"; then
        exit 1
    fi
    grep -Fq '__builtin_mips_ctc2 requires a constant control index 0..31' \
        "$task_out/ctc2-rejected-$task_case.stderr"
done
# The same unsupported-mode positive control above must already have succeeded.
if "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -mcpu=r4000 -mips2 \
    -o "$task_out/ctc2-unsupported.s" "$task_tests/psx_ctc2.c" \
    > "$task_out/ctc2-unsupported.stdout" 2> "$task_out/ctc2-unsupported.stderr"; then
    exit 1
fi
grep -Fq '__builtin_mips_ctc2 requires little-endian MIPS-I' "$task_out/ctc2-unsupported.stderr"
if "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -mpsx-ctc2-return-delay=banana \
    -o "$task_out/ctc2-bad-policy.s" "$task_tests/psx_ctc2.c" \
    > "$task_out/ctc2-bad-policy.stdout" 2> "$task_out/ctc2-bad-policy.stderr"; then
    exit 1
fi
grep -Fq 'bad value (banana)' "$task_out/ctc2-bad-policy.stderr"
grep -Fq 'psx-ctc2-return-delay' "$task_out/ctc2-bad-policy.stderr"

# Zero is a real constant input, not a pseudo for GPR0/the virtual arg pointer.
# Retain all phases so recognized CTC2 MATCH_DUP uses are checked after CSE too.
for task_opt in O0 O2; do
    (cd "$task_out" && "$task_gcc" -B"$task_prefix/" -S "-$task_opt" -G8 \
        -msoft-float -ffunction-sections -da -o "$task_out/zero-$task_opt.s" \
        "$task_tests/psx_ctc2_zero.c")
    for task_name in ctc_zero_literal ctc_zero_folded ctc_zero_repeated \
        ctc_zero_effectful ctc_zero_runtime ctc_zero_nonzero ctc_zero_fifth; do
        task_extract_function "$task_name" "$task_out/zero-$task_opt.s" \
            "$task_out/$task_name-$task_opt.s"
    done
    grep -Eq 'ctc2[[:space:]]+\$0,\$0([[:space:]]|$)' "$task_out/ctc_zero_literal-$task_opt.s"
    grep -Eq 'ctc2[[:space:]]+\$0,\$31([[:space:]]|$)' "$task_out/ctc_zero_folded-$task_opt.s"
    test "$(grep -Ec '^[[:space:]]*ctc2[[:space:]]+\$0,\$[567]([[:space:]]|$)' \
        "$task_out/ctc_zero_repeated-$task_opt.s")" -eq 3
    grep -Eq 'ctc2[[:space:]]+\$0,\$4([[:space:]]|$)' "$task_out/ctc_zero_effectful-$task_opt.s"
    grep -Eq 'ctc2[[:space:]]+\$[1-9][0-9]*,\$4([[:space:]]|$)' "$task_out/ctc_zero_runtime-$task_opt.s"
    grep -Eq 'ctc2[[:space:]]+\$[1-9][0-9]*,\$31([[:space:]]|$)' "$task_out/ctc_zero_nonzero-$task_opt.s"
    grep -Eq 'jal[[:space:]]+ctc_zero_keep_frame' "$task_out/ctc_zero_fifth-$task_opt.s"
    grep -Eq 'ctc2[[:space:]]+\$0,\$6([[:space:]]|$)' "$task_out/ctc_zero_fifth-$task_opt.s"
    task_phases='rtl cse combine greg dbr'
    if test "$task_opt" = O0; then
        test ! -s "$task_out/psx_ctc2_zero.c.dbr"
        task_phases='rtl cse combine greg'
    fi
    for task_phase in $task_phases; do
        awk '
            /\(unspec_volatile\[/ { state=1; next }
            state == 1 && /\(reg|\(const_int/ {
                value=$0; gsub(/[ \t]/, "", value); state=2; next
            }
            state == 2 && /^[ \t]*\][ \t]+[0-9]+\)/ {
                state = ($0 ~ /\][ \t]+4\)/) ? 3 : 0
                next
            }
            state == 3 && /\(use / {
                actual=$0; sub(/^.*\(use /, "", actual); sub(/\)[ \t]*$/, "", actual)
                gsub(/[ \t]/, "", actual)
                if (value != actual) bad=1
                ++pairs; state=0
            }
            END { exit !(pairs && !state && !bad) }
        ' "$task_out/psx_ctc2_zero.c.$task_phase"
    done
done
# Only the intended volatile increment writes memory, before the zero transfer.
test "$(grep -Ec '^[[:space:]]*sw[[:space:]]' "$task_out/ctc_zero_effectful-O2.s")" -eq 1
awk '/^[[:space:]]*sw[[:space:]]/ { store=NR }
     /^[[:space:]]*ctc2[[:space:]]/ { transfer=NR }
     END { exit !(store && transfer && store < transfer) }' "$task_out/ctc_zero_effectful-O2.s"
