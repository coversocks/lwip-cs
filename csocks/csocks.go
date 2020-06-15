package csocks

import (
	"fmt"
	"io"
	"net/url"
	"regexp"
	"strings"
	"sync"

	"github.com/miekg/dns"

	"github.com/coversocks/cs4go/core"

	"github.com/coversocks/lwipcs"
)

//PortDistProcessor impl to core.Processor for distribute processor by target host port
type PortDistProcessor struct {
	handlers map[string]core.Processor
}

//NewPortDistProcessor will create new PortDistProcessor
func NewPortDistProcessor() (p *PortDistProcessor) {
	p = &PortDistProcessor{
		handlers: map[string]core.Processor{},
	}
	return
}

//Add will add processor to handler list
func (p *PortDistProcessor) Add(port string, h core.Processor) {
	p.handlers[port] = h
}

//ProcConn will process connection
func (p *PortDistProcessor) ProcConn(raw io.ReadWriteCloser, target string) (err error) {
	u, err := url.Parse(target)
	if err != nil {
		return
	}
	if h, ok := p.handlers[u.Port()]; ok {
		err = h.ProcConn(raw, target)
		return
	}
	if h, ok := p.handlers["*"]; ok {
		err = h.ProcConn(raw, target)
		return
	}
	err = fmt.Errorf("processor is not exist for %v", target)
	return
}

//SchemeDistProcessor impl to core.Processor for distribute processor by target scheme
type SchemeDistProcessor struct {
	handlers map[string]core.Processor
}

//NewSchemeDistProcessor will create new SchemeDistProcessor
func NewSchemeDistProcessor() (p *SchemeDistProcessor) {
	p = &SchemeDistProcessor{
		handlers: map[string]core.Processor{},
	}
	return
}

//Add will add processor to handler list
func (s *SchemeDistProcessor) Add(scheme string, h core.Processor) {
	s.handlers[scheme] = h
}

//ProcConn will process connection
func (s *SchemeDistProcessor) ProcConn(raw io.ReadWriteCloser, target string) (err error) {
	u, err := url.Parse(target)
	if err != nil {
		return
	}
	if h, ok := s.handlers[u.Scheme]; ok {
		err = h.ProcConn(raw, target)
		return
	}
	err = fmt.Errorf("processor is not exist for %v", target)
	return
}

//DNSGFW impl check if domain in gfw list
type DNSGFW struct {
	list map[string]string
}

//NewDNSGFW will create new GFWList
func NewDNSGFW() (gfw *DNSGFW) {
	gfw = &DNSGFW{
		list: map[string]string{
			"*": "dns://local",
		},
	}
	return
}

//Add list
func (d *DNSGFW) Add(list, target string) {
	d.list[list] = target
}

//Find domain target
func (d *DNSGFW) Find(domain string) (target string) {
	parts := strings.SplitAfterN(domain, ".", 3)
	if len(parts) < 2 {
		target = d.list["*"]
		return
	}
	if len(parts) == 3 {
		parts = parts[1:]
	}
	for i, p := range parts {
		parts[i] = strings.Trim(p, ".")
	}
	ptxt := fmt.Sprintf("[\\|\\.]*%v\\.%v$", parts[0], parts[1])
	pattern := regexp.MustCompile(ptxt)
	for key, val := range d.list {
		// fmt.Printf("testing %v,%v,%v\n", ptxt, key, pattern.MatchString(key))
		if pattern.MatchString(key) {
			target = val
			return
		}
	}
	target = d.list["*"]
	return
}

//DNSConn impl the dns connection for read/write dns message
type DNSConn struct {
	p          *DNSProcessor
	key        string
	base       io.ReadWriteCloser
	readQueued chan []byte
	closed     bool
}

//NewDNSConn will create new DNSConn
func NewDNSConn(p *DNSProcessor, key string, base io.ReadWriteCloser) (conn *DNSConn) {
	conn = &DNSConn{
		p:          p,
		key:        key,
		base:       base,
		readQueued: make(chan []byte, 1024),
	}
	return
}

func (d *DNSConn) Read(p []byte) (n int, err error) {
	data := <-d.readQueued
	if data == nil {
		err = fmt.Errorf("closed")
		return
	}
	if len(data) > len(p) {
		err = fmt.Errorf("buffer is too small")
		return
	}
	n = copy(p, data)
	// fmt.Printf("DNSConn(%p).Read---->%v,%v\n", d, n, data)
	return
}

func (d *DNSConn) Write(p []byte) (n int, err error) {
	// fmt.Printf("DNSConn(%v).Write---->%v,%v\n", d, len(p), p)
	n, err = d.base.Write(p)
	return
}

//Close will close the connection
func (d *DNSConn) Close() (err error) {
	if d.closed {
		err = fmt.Errorf("closed")
		return
	}
	d.closed = true
	close(d.readQueued)
	d.p.close(d)
	return
}

func (d *DNSConn) String() string {
	return fmt.Sprintf("DNSConn(%v)", d.base)
}

//DNSProcessor impl to core.Processor for process dns connection
type DNSProcessor struct {
	conns    map[string]*DNSConn
	connsLck sync.RWMutex
	Next     core.Processor
	Target   func(domain string) string
}

//NewDNSProcessor will create new DNSProcessor
func NewDNSProcessor(next core.Processor, target func(domain string) string) (p *DNSProcessor) {
	p = &DNSProcessor{
		conns:    map[string]*DNSConn{},
		connsLck: sync.RWMutex{},
		Next:     next,
		Target:   target,
	}
	return
}

//ProcConn will process connection
func (d *DNSProcessor) ProcConn(r io.ReadWriteCloser, target string) (err error) {
	go d.proc(r)
	return
}

func (d *DNSProcessor) close(c *DNSConn) {
	d.connsLck.Lock()
	delete(d.conns, c.key)
	d.connsLck.Unlock()
}

func (d *DNSProcessor) proc(r io.ReadWriteCloser) {
	var n int
	var err error
	for {
		buf := make([]byte, 4096)
		n, err = r.Read(buf)
		if err != nil {
			break
		}
		msg := new(dns.Msg)
		err = msg.Unpack(buf[0:n])
		if err != nil {
			break
		}
		var target = d.Target(msg.Question[0].Name)
		var key = fmt.Sprintf("%p-%v", r, target)
		d.connsLck.RLock()
		conn, ok := d.conns[key]
		d.connsLck.RUnlock()
		if !ok {
			conn = NewDNSConn(d, key, r)
			err = d.Next.ProcConn(conn, target)
			if err != nil {
				//drop it
				continue
			}
			d.connsLck.Lock()
			d.conns[key] = conn
			d.connsLck.Unlock()
		}
		// fmt.Printf("DNSProcessor(%v).Queued %p---->%v,%v\n", conn, conn, n, buf[0:n])
		conn.readQueued <- buf[0:n]
	}
}

//DNSRecordConn is dns connection for recording dns response
type DNSRecordConn struct {
	p    *DNSRecordProcessor
	base io.ReadWriteCloser
}

//NewDNSRecordConn will create new DNSRecordConn
func NewDNSRecordConn(p *DNSRecordProcessor, base io.ReadWriteCloser) (conn *DNSRecordConn) {
	conn = &DNSRecordConn{
		p:    p,
		base: base,
	}
	return
}

func (d *DNSRecordConn) Read(p []byte) (n int, err error) {
	n, err = d.base.Read(p)
	// fmt.Printf("DNSRecordConn(%v).Read---->%v,%v\n", d, n, p[0:n])
	return
}

func (d *DNSRecordConn) Write(p []byte) (n int, err error) {
	// fmt.Printf("DNSRecordConn(%v).Write---->%v,%v\n", d, len(p), p)
	msg := new(dns.Msg)
	if xerr := msg.Unpack(p); xerr == nil && len(msg.Answer) > 0 {
		for _, answer := range msg.Answer {
			if a, ok := answer.(*dns.A); ok {
				core.DebugLog("DNSRecord recoding %v->%v", a.Hdr.Name, a.A)
				d.p.Record(a.A.String(), a.Hdr.Name)
			}
		}
	}
	n, err = d.base.Write(p)
	return
}

//Close will close base connection
func (d *DNSRecordConn) Close() (err error) {
	err = d.base.Close()
	core.DebugLog("%v is closed", d)
	return
}

func (d *DNSRecordConn) String() string {
	return fmt.Sprintf("DNSRecordConn(%v)", d.base)
}

//DNSRecordProcessor to impl processor for record dns response
type DNSRecordProcessor struct {
	Next   core.Processor
	allIP  map[string]string
	allLck sync.RWMutex
}

//NewDNSRecordProcessor will create new DNSRecordProcessor
func NewDNSRecordProcessor(next core.Processor) (r *DNSRecordProcessor) {
	r = &DNSRecordProcessor{
		Next:   next,
		allIP:  map[string]string{},
		allLck: sync.RWMutex{},
	}
	return
}

//Record the key and value
func (d *DNSRecordProcessor) Record(key, val string) {
	d.allLck.Lock()
	d.allIP[key] = val
	d.allLck.Unlock()
}

//IsRecorded will check if key is recorded
func (d *DNSRecordProcessor) IsRecorded(key string) (ok bool) {
	d.allLck.RLock()
	_, ok = d.allIP[key]
	d.allLck.RUnlock()
	return
}

//Clear all recorded
func (d *DNSRecordProcessor) Clear() {
	d.allLck.Lock()
	d.allIP = map[string]string{}
	d.allLck.Unlock()
}

//ProcConn will process connection
func (d *DNSRecordProcessor) ProcConn(r io.ReadWriteCloser, target string) (err error) {
	err = d.Next.ProcConn(NewDNSRecordConn(d, r), target)
	return
}

//PACProcessor to impl core.Processor for pac
type PACProcessor struct {
	Proxy  core.Processor
	Direct core.Processor
	Record *DNSRecordProcessor
}

//NewPACProcessor will create new PACProcessor
func NewPACProcessor(proxy, direct core.Processor) (pac *PACProcessor) {
	pac = &PACProcessor{
		Proxy:  proxy,
		Direct: direct,
	}
	return
}

//ProcConn will process connection
func (p *PACProcessor) ProcConn(r io.ReadWriteCloser, target string) (err error) {
	u, err := url.Parse(target)
	if err != nil {
		return
	}
	if u.Host == "proxy" || (p.Record != nil && p.Record.IsRecorded(u.Hostname())) {
		err = p.Proxy.ProcConn(r, target)
	} else {
		err = p.Direct.ProcConn(r, target)
	}
	return
}

//LwipHandler is impl for lwipcs.Event
type LwipHandler struct {
	Next core.Processor
	GFW  *DNSGFW
}

//NewLwipHandler will create new LwipHandler
func NewLwipHandler(proxy, direct core.Processor) (handler *LwipHandler) {
	gfw := NewDNSGFW()
	pac := NewPACProcessor(proxy, direct)
	record := NewDNSRecordProcessor(pac)
	pac.Record = record
	dns := NewDNSProcessor(record, gfw.Find)
	port := NewPortDistProcessor()
	port.Add("53", dns)
	port.Add("*", pac)
	scheme := NewSchemeDistProcessor()
	scheme.Add("tcp", pac)
	scheme.Add("udp", port)
	handler = &LwipHandler{Next: scheme, GFW: gfw}
	return
}

//OnAccept will handler the tcp accept
func (l *LwipHandler) OnAccept(pcb *lwipcs.PCB) {
	core.DebugLog("LWIP accept %v from %v to %v", pcb.Type, pcb.RemoteAddr(), pcb.LocalAddr())
	err := l.Next.ProcConn(pcb, pcb.Type+"://"+pcb.LocalAddr().String())
	if err == nil {
		core.DebugLog("LWIP accept %v from %v to %v success", pcb.Type, pcb.RemoteAddr(), pcb.LocalAddr())
	} else {
		core.DebugLog("LWIP accept %v from %v to %v fail with %v", pcb.Type, pcb.RemoteAddr(), pcb.LocalAddr(), err)
	}
}

//OnClose will handler the tcp close
func (l *LwipHandler) OnClose(pcb *lwipcs.PCB) {
	core.DebugLog("LWIP tcp close from %v", pcb.RemoteAddr())
}
