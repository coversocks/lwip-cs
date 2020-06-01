package main

import "C"
import (
	"io"
	"net"

	"github.com/coversocks/cs4go/core"
	"github.com/coversocks/lwipcs"
	"github.com/coversocks/lwipcs/csocks"
)

//ProxyDialer is test proxy dialer
type ProxyDialer struct {
}

//ProcConn will process connection
func (p *ProxyDialer) ProcConn(r io.ReadWriteCloser, target string) (err error) {
	panic("not supported")
}

//export go_cs_proc
func go_cs_proc() {
	core.SetLogLevel(core.LogLevelDebug)
	udpDialer := &net.Dialer{}
	udpDialer.LocalAddr = &net.UDPAddr{IP: net.ParseIP("192.168.0.103")}
	tcpDialer := &net.Dialer{}
	tcpDialer.LocalAddr = &net.TCPAddr{IP: net.ParseIP("192.168.0.103")}
	direct := core.NewNetDialer("8.8.4.4:53")
	direct.UDP = udpDialer
	direct.TCP = tcpDialer
	proxy := &ProxyDialer{}
	lwipcs.Event = csocks.NewLwipHandler(proxy, direct)
	go go_cs_read()
	lwipcs.Proc()
}

//export go_cs_read
func go_cs_read() {
	lwipcs.Read()
}

func main() {

}
