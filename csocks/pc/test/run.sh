set -e
rm -f libcsocks.a libcsocks.h test
go build --buildmode=c-archive -o libcsocks.a github.com/coversocks/lwipcs/csocks/pc
gcc test.c -I ../../../ -I../../../../lwip/src/include -I../../../src -lcsocks -llwipcs -llwipcore -L./ -L../../../build/src/ -L../../../../lwip/build/contrib/ports/unix/example_app -pthread -o test
sudo ./test
