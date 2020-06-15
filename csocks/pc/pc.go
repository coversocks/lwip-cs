package main

import "C"
import (
	"fmt"
	"io"
	"log"
	"net"
	"time"

	"github.com/coversocks/cs4go"
	"github.com/coversocks/cs4go/core"
	"github.com/coversocks/lwipcs"
	"github.com/coversocks/lwipcs/csocks"

	"net/http"
	_ "net/http/pprof"
)

func init() {
	go func() {
		log.Println(http.ListenAndServe(":6060", nil))
	}()
}

//ProxyDialer is test proxy dialer
type ProxyDialer struct {
}

//ProcConn will process connection
func (p *ProxyDialer) ProcConn(r io.ReadWriteCloser, target string) (err error) {
	panic("not supported")
}

//EchoConn is impl for net.Conn
type EchoConn struct {
	buf    chan []byte
	closed bool
}

//NewEchoConn will return new echo connection
func NewEchoConn() (c *EchoConn) {
	c = &EchoConn{
		buf: make(chan []byte, 1024),
	}
	return
}

func (e *EchoConn) Read(b []byte) (n int, err error) {
	if e.closed {
		err = fmt.Errorf("closed")
		return
	}
	c := <-e.buf
	if c == nil {
		err = fmt.Errorf("closed")
		return
	}
	if len(b) < len(c) {
		panic("buffer too small")
	}
	n = copy(b, c)
	return
}

func (e *EchoConn) Write(b []byte) (n int, err error) {
	if e.closed {
		err = fmt.Errorf("closed")
		return
	}
	e.buf <- b
	n = len(b)
	return
}

// Close closes the connection.
func (e *EchoConn) Close() error {
	e.closed = true
	close(e.buf)
	return nil
}

// LocalAddr returns the local network address.
func (e *EchoConn) LocalAddr() net.Addr {
	return nil
}

// RemoteAddr returns the remote network address.
func (e *EchoConn) RemoteAddr() net.Addr {
	return nil
}

// SetDeadline for net.Conn
func (e *EchoConn) SetDeadline(t time.Time) error {
	return nil
}

// SetReadDeadline for net.Conn
func (e *EchoConn) SetReadDeadline(t time.Time) error {
	return nil
}

// SetWriteDeadline for net.Conn
func (e *EchoConn) SetWriteDeadline(t time.Time) error {
	return nil
}

//EchoDialer is dialer test
type EchoDialer struct {
}

//Dial dail one raw connection
func (e *EchoDialer) Dial(network, address string) (c net.Conn, err error) {
	if address == "114.114.114.114:53" {
		c, err = net.Dial(network, address)
	} else {
		c = NewEchoConn()
	}
	return
}

//export go_cs_proc
func go_cs_proc(c string) {
	core.SetLogLevel(core.LogLevelDebug)
	conf := cs4go.ClientConf{Mode: "auto"}
	err := core.ReadJSON(c, &conf)
	if err != nil {
		core.ErrorLog("Client read configure fail with %v", err)
		return
	}
	core.SetLogLevel(conf.LogLevel)
	core.InfoLog("Client using config from %v", c)
	client := &cs4go.Client{Conf: conf}
	client.Boostrap(core.NewWebsocketDialer())
	udpDialer := &net.Dialer{
		LocalAddr: &net.UDPAddr{IP: net.ParseIP("172.17.0.2")},
		Timeout:   5 * time.Second,
	}
	tcpDialer := &net.Dialer{
		LocalAddr: &net.TCPAddr{IP: net.ParseIP("172.17.0.2")},
		Timeout:   5 * time.Second,
	}
	// udpDialer := &EchoDialer{}
	// tcpDialer := &EchoDialer{}
	direct := core.NewNetDialer("114.114.114.114:53")
	direct.UDP = udpDialer
	direct.TCP = tcpDialer
	// proxy := &ProxyDialer{}
	handler := csocks.NewLwipHandler(core.NewAyncProcessor(client), core.NewAyncProcessor(core.NewProcConnDialer(direct)))
	// handler.GFW.Add("||google.com", "dns://proxy")
	lwipcs.Event = handler
	go go_cs_read()
	lwipcs.Proc()
}

//export go_cs_read
func go_cs_read() {
	lwipcs.Read()
}

func main() {

}
