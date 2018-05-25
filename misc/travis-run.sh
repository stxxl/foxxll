#/bin/bash

ret=0
echo "OSTYPE: $OSTYPE"

for t in "syscall unlink" "linuxaio unlink" "memory"; do
    echo -e "\n\n\e[32m\e[21mTest IO subsystem: $t\n========================================================\e[0m\n"
    if [[ "$t" == "linuxaio unlink" && "$OSTYPE" == "darwin"* ]]; then
        echo "Skipping on Mac OS"
        continue
    fi
    echo "disk=${FOXXLL_TEST_DISKDIR}/stxxl.$$.tmp,256Mi,$t" > ~/.stxxl
    ./tools/foxxll_tool info
    ctest -V || ret=$?
done

echo "Test result: $ret"
exit $ret
