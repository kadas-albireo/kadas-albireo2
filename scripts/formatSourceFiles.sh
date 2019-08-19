#!/bin/sh
dir=$(readlink -f "$(dirname "$(readlink -f "$0")")/..")
which astyle &> /dev/null || ( echo "Please install astyle." && exit 1 )

out=$(astyle --formatted --options=$dir/scripts/astyle.options $(find $dir/kadas \( -name '*.cpp' -or -name '*.h' \)))
echo "$out"
[ -z "$out" ] && exit 0 || exit 1
