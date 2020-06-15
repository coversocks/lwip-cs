set -e
rm -f libcsocks.a libcsocks.h
rm -f test
export GODEBUG=cgocheck=2
go install github.com/coversocks/lwipcs
go build --buildmode=c-archive -o libcsocks.a github.com/coversocks/lwipcs/csocks/pc

sysname="$(uname -s)"
case "${sysname}" in
    Linux*)
    gcc test.c -g -I ../../../ -I../../../../lwip/src/include -I../../../src -lcsocks -llwipcore -llwipcs -llwipcore -L./ -L../../../build/src/ -L../../../../lwip-contrib/ports/unix/example_app/build -pthread -o test
    ;;
    Darwin*)
    gcc test.c -framework Security -framework Foundation -framework AppKit -I ../../../ -I../../../../lwip/src/include -I../../../src -lcsocks -llwipcs -llwipcore -L./ -L../../../build/src/ -L../../../../lwip/build/contrib/ports/unix/example_app -pthread -o test
    ;;
    MINGW*)
    ;;
esac
# ./test
