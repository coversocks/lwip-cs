package lwipcs

import (
	"fmt"
	"net"
	"unsafe"
)

// #cgo amd64 386 CFLAGS: -I../lwip/src/include/ -I./src -I../lwip/contrib/ports/unix/port/include
// #cgo amd64 386 LDFLAGS: -llwipcs -llwipcore -L ./build/src/ -L../lwip/build/contrib/ports/unix/example_app
// #cgo ios CFLAGS: -I../lwip/src/include/ -I./src -I../lwip/contrib/ports/unix/port/include
// #cgo ios LDFLAGS: -llwipcs -llwipcore -L /Users/vty/deps/ios/arm64/lib/
// #include "src/cs.h"
// int go_cs_tcp_send(void *tpcb_, void *state_, char *buf, int buf_len);
// int go_cs_tcp_close(void *tpcb_, void *state_);
// int go_cs_udp_send(void *upcb_, ip_addr_t *laddr, u16_t lport, ip_addr_t *raddr, uint16_t rport, char *buf, int buf_len);
// ip_addr_t go_cs_tcp_local_ip(void *tpcb_);
// int go_cs_tcp_local_port(void *tpcb_);
// ip_addr_t go_cs_tcp_remote_ip(void *tpcb_);
// int go_cs_tcp_remote_port(void *tpcb_);
// ip_addr_t go_cs_udp_local_ip(void *upcb_);
// int go_cs_udp_local_port(void *upcb_);
// int go_cs_ip_len(ip_addr_t *addr_);
// void go_cs_ip_get(ip_addr_t *addr_, char *buf);
// int go_cs_pbuf_len(void *p_);
// void *go_cs_pbuf_alloc(u16_t len);
// void go_cs_pbuf_take(void *p_, char *buf);
// void go_cs_pbuf_copy(void *p_, char *buf);
// void *go_cs_netif_init(void *back_);
// void *go_cs_netif_deinit(void *netif_);
// void go_cs_netif_proc(void *netif_);
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
	recevied   []byte
	recvQueued chan []byte
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
		recevied:   nil,
	}
	if ptype == TCP {
		p.recvQueued = make(chan []byte, 10240)
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
	for {
		if len(p.recevied) < 1 {
			p.recevied = <-p.recvQueued
			if p == nil {
				err = fmt.Errorf("%v", "closed")
				return
			}
		}
		copy(b[l:], p.recevied)
		if len(b) <= len(p.recevied) {
			l += len(b)
			p.recevied = p.recevied[:l]
			return
		}
		l += len(p.recevied)
		p.recevied = nil
		select {
		case p.recevied = <-p.recvQueued:
			if p == nil {
				err = fmt.Errorf("%v", "closed")
				return
			}
		default:
			return
		}
	}
}

func (p *PCB) Write(b []byte) (l int, err error) {
	if err == nil {
		// <-p.sendQueued //wait sended
		sendQueue <- &sendEntry{PCB: p, Data: b}
	}
	l = len(b)
	err = p.err
	return
}

//Close will close pcb
func (p *PCB) Close() (err error) {
	if p.err != nil {
		return p.err
	}
	p.err = fmt.Errorf("%v", "closed")
	if p.Type == TCP {
		close(p.recvQueued)
		closeQueue <- p
	}
	return
}

var pcbs = map[uintptr]*PCB{}

//export go_tcp_accept_h
func go_tcp_accept_h(arg, newpcb, state unsafe.Pointer) int {
	if newpcb == nil {
		return 1
	}
	var pcb = pcbs[uintptr(newpcb)]
	if pcb == nil {
		pcb = newPCB(TCP, arg, newpcb, state,
			C.go_cs_tcp_local_ip(newpcb), C.go_cs_tcp_remote_ip(newpcb),
			int(C.go_cs_tcp_local_port(newpcb)), int(C.go_cs_tcp_remote_port(newpcb)),
		)
		pcbs[uintptr(newpcb)] = pcb
	}
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
	var pcb = pcbs[uintptr(tpcb)]
	if pcb == nil {
		return 1
	}
	var data = make([]byte, C.go_cs_pbuf_len(buf))
	C.go_cs_pbuf_copy(buf, (*C.char)(unsafe.Pointer(&data[0])))
	pcb.recvQueued <- data
	return 0
}

//export go_tcp_send_done_h
func go_tcp_send_done_h(arg, tpcb, state, buf unsafe.Pointer) int {
	// if tpcb == nil {
	// 	return 1
	// }
	// var pcb = pcbs[uintptr(tpcb)]
	// if pcb == nil {
	// 	return 1
	// }
	// pcb.sendQueued <- 1
	return 0
}

//export go_tcp_close_h
func go_tcp_close_h(arg, tpcb, state unsafe.Pointer) int {
	if tpcb == nil {
		return 1
	}
	var pcb = pcbs[uintptr(tpcb)]
	if pcb == nil {
		return 1
	}
	if pcb.err == nil {
		pcb.Close()
	}
	if Event != nil {
		Event.OnClose(pcb)
	}
	return 0
}

func tcpSend(tpcb, state unsafe.Pointer, buf []byte) int {
	var ret = C.go_cs_tcp_send(tpcb, state, (*C.char)(unsafe.Pointer(&buf)), C.int(len(buf)))
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
	if s.PCB.Type == TCP {
		tcpSend(s.PCB.pcb, s.PCB.state, s.Data)
	} else {
		udpSend(s.PCB.pcb, &s.PCB.localIP, s.PCB.localPort, &s.PCB.remoteIP, s.PCB.remotePort, s.Data)
	}
}

var sendQueue = make(chan *sendEntry, 1000)
var closeQueue = make(chan *PCB, 1000)
var runnning = false

//Event will proc all event
var Event Handler

//Init will init the netif
func Init(back unsafe.Pointer) unsafe.Pointer {
	return C.go_cs_netif_init(back)
}

//Deinit will free netif
func Deinit(netif unsafe.Pointer) {
	C.go_cs_netif_deinit(netif)
}

//Proc will proc netif input
func Proc(netif unsafe.Pointer) {
	runnning = true
	for runnning {
		select {
		case c := <-closeQueue:
			tcpClose(c.pcb, c.state)
		case s := <-sendQueue:
			s.Send()
		default:
			C.go_cs_netif_proc(netif)
		}
	}
}

//Stop running proc
func Stop() {
	runnning = false
}
