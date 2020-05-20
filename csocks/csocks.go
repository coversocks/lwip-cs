package csocks

import (
	"fmt"
	"io"
	"log"
	"strings"

	"github.com/coversocks/lwipcs"

	_ "net/http/pprof"

	"net/http"
)

type Echo struct {
	rrr    bool
	recved uint64
	begin  int64
}

func (e *Echo) Show() {
	// for {
	// 	fmt.Printf("R(%v)\n", e.recved)
	// 	time.Sleep(time.Second)
	// }
}

func (e *Echo) OnAccept(pcb *lwipcs.PCB) {
	fmt.Printf("accept from %v\n", pcb.RemoteAddr())
	go func() {
		io.Copy(pcb, pcb)
		// io.Copy(ioutil.Discard, pcb)
		fmt.Printf("copy done for %v\n", pcb.RemoteAddr())
	}()
	// go func() {
	// 	buf := make([]byte, 1024)
	// 	for {
	// 		r, err := pcb.Read(buf)
	// 		if err != nil {
	// 			break
	// 		}
	// 		atomic.AddUint64(&e.recved, uint64(r))
	// 	}
	// 	pcb.Close()
	// 	fmt.Printf("read done for %v\n", pcb.RemoteAddr())
	// }()
	// go func() {
	// 	// buf := make([]byte, 1024)
	// 	data := fmt.Sprintf("send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0send-0")
	// 	for {
	// 		_, err := pcb.Write([]byte(data))
	// 		if err != nil {
	// 			break
	// 		}
	// 		// time.Sleep(10000 * time.Microsecond)
	// 	}
	// 	pcb.Close()
	// 	fmt.Printf("write done for %v\n", pcb.RemoteAddr())
	// }()
	// go func() {
	// 	// buf := make([]byte, 1024)
	// 	data := fmt.Sprintf("send-0")
	// 	for {
	// 		// r, err := pcb.Read(buf)
	// 		// if err != nil {
	// 		// 	break
	// 		// }
	// 		// fmt.Printf("val(%v):%v\n", r, string(buf[0:r]))
	// 		_, err := pcb.Write([]byte(data))
	// 		if err != nil {
	// 			break
	// 		}
	// 	}
	// 	pcb.Close()
	// }()
}
func (e *Echo) OnClose(pcb *lwipcs.PCB) {
	fmt.Printf("close for %v\n", pcb.RemoteAddr())
}
func (e *Echo) OnRecv(pcb *lwipcs.PCB, data []byte) {
	if strings.Contains(pcb.RemoteAddr().String(), ":5353") {
		return
	}
	fmt.Printf("udp from %v\n", pcb.RemoteAddr())
	fmt.Printf("udp recv %v\n", data)
	pcb.Write(data)
}

func init() {
	echo := &Echo{}
	go echo.Show()
	go func() {
		log.Println(http.ListenAndServe("localhost:6060", nil))
	}()
	lwipcs.Event = echo
}

func Proc() {
	lwipcs.Proc()
}

func Read() {
	lwipcs.Read()
}
