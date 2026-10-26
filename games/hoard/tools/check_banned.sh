#!/usr/bin/env bash
# Determinism gate for sim/: exit 1 and list the lines that use anything banned by the policy
# (docs/research-determinism-ci.md section 1). The vendored json.h and the own math in core/detmath.* are exempt.
set -uo pipefail
cd "$(dirname "$0")/.."
files=$(find sim/ages sim/core sim/wasm -name '*.cpp' -o -name '*.h' | grep -v -e 'sim/ages/json.h' -e 'sim/core/detmath\.')
math='exp|exp2|expm1|log|log2|log10|log1p|pow|sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh|asinh|acosh|atanh|cbrt|hypot|erf|erfc|tgamma|lgamma|fma|fmod|remainder|nextafter'
fail=0
check() {
    local what="$1" pattern="$2" out
    out=$(grep -nP -- "$pattern" $files | grep -v '^\S*:[0-9]*:\s*//')
    if [ -n "$out" ]; then echo "banned: $what"; echo "$out"; fail=1; fi
}
check "libm function (use det::)" "(std::($math)\s*\()|((?<![\w.:>])($math)\s*\()"
check "float" '\bfloat\b'
check "long double" '\blong\s+double\b'
check "<random>" '#include\s*<random>'
check "std::shuffle / std::reduce / std::hash" 'std::(shuffle|reduce|hash)\b'
check "rand / time / clock" '(?<![\w.:>])(rand|srand|time|clock)\s*\('
check "threads" '#include\s*<(thread|future|execution)>|std::(thread|async)\b'
check "unordered containers" '\bunordered_(map|set|multimap|multiset)\b'
check "two rng draws in one expression" 'rng\.\w+\(.*rng\.\w+\('
check "fast-math flags" '-ffast-math|-Ofast|/fp:fast'
exit $fail
