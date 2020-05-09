package lwipcs

// #cgo CFLAGS: -I../lwip/src/include/ -I./src -I../lwip/contrib/ports/unix/port/include
// #cgo LDFLAGS: -llwipcs -llwipcore -L ./build/src/ -L../lwip/build/contrib/ports/unix/example_app
// extern int cs_tcp_send(void *tpcb_, void *state_, char *buf, int buf_len);
// extern int cs_udp_send(void *upcb_, void *addr_, int port, char *buf, int buf_len);
import "C"
import "unsafe"

type PCB struct {
	pcbType  int
	State    int
	Src      []byte
	SrcPort  int
	Dest     []byte
	DestPort int
	Data     []byte
}

//export tcpAccept
func tcpAccept(arg, newpcb, state unsafe.Pointer,
	addrLen int, src unsafe.Pointer, srcPort int, dest unsafe.Pointer, destPort int) int {
	return 0
}

//export tcpRecv
func tcpRecv(arg, tpcb, state unsafe.Pointer,
	addrLen int, src unsafe.Pointer, srcPort int, dest unsafe.Pointer, destPort int,
	buf unsafe.Pointer, bufLen int) int {

	return 0
}

//export tcpClose
func tcpClose(arg, tpcb, state unsafe.Pointer,
	addrLen int, src unsafe.Pointer, srcPort int, dest unsafe.Pointer, destPort int) int {
	return 0
}

func tcpSend(tpcb, state unsafe.Pointer, buf []byte) int {
	var ret = C.cs_tcp_send(tpcb, state, (*C.char)(unsafe.Pointer(&buf)), C.int(len(buf)))
	return int(ret)
}

//export udpRecv
func udpRecv(arg, upcb, state unsafe.Pointer,
	addrLen int, src unsafe.Pointer, srcPort int, dest unsafe.Pointer, destPort int,
	buf unsafe.Pointer, bufLen int) int {

	return 0
}

func udpSend(upcb, addr unsafe.Pointer, port int, buf []byte) int {
	var ret = C.cs_udp_send(upcb, addr, C.int(port), (*C.char)(unsafe.Pointer(&buf)), C.int(len(buf)))
	return int(ret)
}
