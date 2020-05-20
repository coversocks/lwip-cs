package lwipcs

import (
	"fmt"
	"net"
	"unsafe"
)

// #cgo amd64 386 CFLAGS: -I../lwip/src/include/ -I./src -I../lwip/contrib/ports/unix/port/include
// #cgo amd64 386 LDFLAGS: -L ./build/src/ -L../lwip/build/contrib/ports/unix/example_app
// #cgo ios CFLAGS: -I../lwip/src/include/ -I./src -I../lwip/contrib/ports/unix/port/include
// #cgo ios LDFLAGS: -L /Users/vty/deps/ios/arm64/lib/
// #cgo android CFLAGS: -I../lwip/src/include/ -I./src -I../lwip/contrib/ports/unix/port/include
// #cgo android LDFLAGS: -L /Users/vty/deps/ios/arm64/lib/
// #define _CGO_BUILD_
// #include "lwipcs.h"
import "C"

//Handler is the interface for lwipcs event.
type Handler interface {
	OnAccept(pcb *PCB)
	OnClose(pcb *PCB)
	OnRecv(pcb *PCB, data []byte)
}

const (
	//TCP is the pcb type for tcp connection
	TCP = 100
	//UDP is the pcb type for udp connection
	UDP = 200
)

//PCB is the connection pcb
type PCB struct {
	Type       int
	arg, pcb   unsafe.Pointer
	state      unsafe.Pointer
	localIP    C.ip_addr_t
	localPort  int
	remoteIP   C.ip_addr_t
	remotePort int
	port       int
	err        error
	recvQueued chan []byte
	sendWait   chan int
}

//newPCB will create new pcb
func newPCB(ptype int, arg, pcb, state unsafe.Pointer, laddr, raddr C.ip_addr_t, lport, rport int) *PCB {
	var p = &PCB{
		Type:       ptype,
		arg:        arg,
		pcb:        pcb,
		state:      state,
		localIP:    laddr,
		localPort:  lport,
		remoteIP:   raddr,
		remotePort: rport,
	}
	if ptype == TCP {
		p.recvQueued = make(chan []byte, 1024)
		p.sendWait = make(chan int, 1)
	}
	return p
}

//LocalAddr return local address
func (p *PCB) LocalAddr() (addr net.Addr) {
	if p.Type == 100 { //tcp
		var local = &p.localIP
		var ip = make([]byte, C.go_cs_ip_len(local))
		C.go_cs_ip_get(local, (*C.char)(unsafe.Pointer(&ip[0])))
		return &net.TCPAddr{IP: net.IP(ip), Port: p.localPort}
	}
	var local = &p.localIP
	var ip = make([]byte, C.go_cs_ip_len(local))
	C.go_cs_ip_get(local, (*C.char)(unsafe.Pointer(&ip[0])))
	return &net.UDPAddr{IP: net.IP(ip), Port: p.localPort}
}

//RemoteAddr return local address
func (p *PCB) RemoteAddr() (addr net.Addr) {
	if p.Type == 100 { //tcp
		var remote = &p.remoteIP
		var ip = make([]byte, C.go_cs_ip_len(remote))
		C.go_cs_ip_get(remote, (*C.char)(unsafe.Pointer(&ip[0])))
		return &net.TCPAddr{IP: net.IP(ip), Port: p.remotePort}
	}
	var remote = &p.remoteIP
	var ip = make([]byte, C.go_cs_ip_len(remote))
	C.go_cs_ip_get(remote, (*C.char)(unsafe.Pointer(&ip[0])))
	return &net.UDPAddr{IP: net.IP(ip), Port: p.remotePort}
}

func (p *PCB) Read(b []byte) (l int, err error) {
	if p.Type != TCP {
		panic("not readable")
	}
	if p.err != nil {
		err = p.err
		return
	}
	recevied := <-p.recvQueued
	if recevied == nil || len(recevied) < 1 {
		err = p.err
		return
	}
	if len(b) < len(recevied) {
		panic(fmt.Sprintf("the read buffer is too small, exprect at least 1518"))
	}
	l = copy(b, recevied)
	recvedQueue <- &recvedEntry{PCB: p, Len: l}
	return
}

func (p *PCB) Write(b []byte) (l int, err error) {
	if len(b) < 1 {
		err = fmt.Errorf("data is empty")
		return
	}
	if p.err == nil {
		sendQueue <- &sendEntry{PCB: p, Data: b}
		<-p.sendWait
	}
	l = len(b)
	err = p.err
	return
}

//Close will close pcb
func (p *PCB) Close() (err error) {
	err = p.close()
	closeQueue <- p
	return
}

func (p *PCB) close() (err error) {
	if p.err != nil {
		return p.err
	}
	p.err = fmt.Errorf("%v", "closed")
	if p.Type == TCP {
		close(p.recvQueued)
		close(p.sendWait)
	}
	return
}

var pcbs = map[uintptr]*PCB{}

//export go_tcp_accept_h
func go_tcp_accept_h(arg, newpcb, state unsafe.Pointer) int {
	if newpcb == nil {
		return 1
	}
	var pcb = pcbs[uintptr(state)]
	if pcb != nil {
		fmt.Printf("raw accept error by %p\n", newpcb)
		return 1
	}
	pcb = newPCB(TCP, arg, newpcb, state,
		C.go_cs_tcp_local_ip(newpcb), C.go_cs_tcp_remote_ip(newpcb),
		int(C.go_cs_tcp_local_port(newpcb)), int(C.go_cs_tcp_remote_port(newpcb)),
	)
	pcbs[uintptr(state)] = pcb
	if Event != nil {
		Event.OnAccept(pcb)
	}
	return 0
}

//export go_tcp_recv_h
func go_tcp_recv_h(arg, tpcb, state, buf unsafe.Pointer) int {
	if tpcb == nil || buf == nil {
		return 1
	}
	var pcb = pcbs[uintptr(state)]
	if pcb == nil || pcb.err != nil {
		return 1
	}
	var data = make([]byte, C.go_cs_pbuf_len(buf))
	C.go_cs_pbuf_copy(buf, (*C.char)(unsafe.Pointer(&data[0])))
	pcb.recvQueued <- data
	return 0
}

//export go_tcp_send_done_h
func go_tcp_send_done_h(arg, tpcb, state, buf unsafe.Pointer) int {
	if tpcb == nil {
		return 1
	}
	var pcb = pcbs[uintptr(state)]
	if pcb == nil {
		return 1
	}
	if pcb.err != nil {
		return 1
	}
	pcb.sendWait <- 1
	return 0
}

//export go_tcp_close_h
func go_tcp_close_h(arg, tpcb, state unsafe.Pointer) int {
	if tpcb == nil {
		return 1
	}
	var pcb = pcbs[uintptr(state)]
	if pcb == nil {
		return 1
	}
	delete(pcbs, uintptr(state))
	pcb.close()
	if Event != nil {
		Event.OnClose(pcb)
	}
	return 0
}

func tcpSend(tpcb, state unsafe.Pointer, buf []byte) int {
	var ret = C.go_cs_tcp_send(tpcb, state, (*C.char)(unsafe.Pointer(&buf[0])), C.int(len(buf)))
	return int(ret)
}

func tcpClose(tpcb, state unsafe.Pointer) int {
	var ret = C.go_cs_tcp_close(tpcb, state)
	return int(ret)
}

//export go_udp_recv_h
func go_udp_recv_h(arg, upcb unsafe.Pointer, addr *C.ip_addr_t, port int, buf unsafe.Pointer) int {
	if Event != nil {
		var data = make([]byte, C.go_cs_pbuf_len(buf))
		C.go_cs_pbuf_copy(buf, (*C.char)(unsafe.Pointer(&data[0])))
		var pcb = newPCB(UDP, arg, upcb, nil,
			C.go_cs_udp_local_ip(upcb), *addr,
			int(C.go_cs_udp_local_port(upcb)), port,
		)
		Event.OnRecv(pcb, data)
	}
	return 0
}

func udpSend(upcb unsafe.Pointer, laddr *C.ip_addr_t, lport int, raddr *C.ip_addr_t, rport int, buf []byte) int {
	var ret = C.go_cs_udp_send(upcb, laddr, C.u16_t(lport), raddr, C.u16_t(rport), (*C.char)(unsafe.Pointer(&buf[0])), C.int(len(buf)))
	return int(ret)
}

type sendEntry struct {
	PCB  *PCB
	Data []byte
}

func (s *sendEntry) Send() {
	if s.PCB.err != nil {
		return
	}
	if s.PCB.Type == TCP {
		tcpSend(s.PCB.pcb, s.PCB.state, s.Data)
	} else {
		udpSend(s.PCB.pcb, &s.PCB.localIP, s.PCB.localPort, &s.PCB.remoteIP, s.PCB.remotePort, s.Data)
	}
}

type recvedEntry struct {
	PCB *PCB
	Len int
}

var inputQueue = make(chan []byte, 1024)
var recvedQueue = make(chan *recvedEntry, 1024)
var sendQueue = make(chan *sendEntry, 1024)
var closeQueue = make(chan *PCB, 1024)

//export go_input_h
func go_input_h(arg, netif unsafe.Pointer, readlen *C.u16_t) unsafe.Pointer {
	for {
		select {
		case r := <-recvedQueue:
			if r.PCB.err == nil {
				C.go_cs_tcp_recved(r.PCB.pcb, C.u16_t(r.Len))
			}
		case c := <-closeQueue:
			tcpClose(c.pcb, c.state)
		case s := <-sendQueue:
			s.Send()
		case i := <-inputQueue:
			if i == nil || len(i) < 1 {
				return nil
			}
			*readlen = C.u16_t(len(i))
			var p = C.go_cs_pbuf_alloc(*readlen)
			if p != nil {
				C.go_cs_pbuf_take(p, (*C.char)(unsafe.Pointer(&i[0])))
			}
			return p
		}
	}
	// return nil
}

var runnning = false

//Event will proc all event
var Event Handler

//export go_netif_proc
func go_netif_proc() {
	runnning = true
	var netif = C.go_cs_netif_get()
	for runnning {
		C.go_cs_netif_proc(netif)
		// select {
		// case c := <-closeQueue:
		// 	tcpClose(c.pcb, c.state)
		// 	C.go_cs_netif_proc(netif)
		// case s := <-sendQueue:
		// 	s.Send()
		// 	C.go_cs_netif_proc(netif)
		// default:
		// 	C.go_cs_netif_proc(netif)
		// }
	}
}

//export go_netif_read
func go_netif_read() {
	runnning = true
	for runnning {
		var buf = make([]byte, 1518)
		var readed = C.go_cs_input((*C.char)(unsafe.Pointer(&buf[0])), 1518)
		if readed < 1 {
			break
		}
		inputQueue <- buf[0:readed]
	}
}

//Proc will run the netif proc
func Proc() {
	go_netif_proc()
}

//Read will run the netif read
func Read() {
	go_netif_read()
}
