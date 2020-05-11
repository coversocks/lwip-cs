package csocks

import (
	"fmt"
	"io"
	"unsafe"

	"github.com/coversocks/lwipcs"
)

type Echo struct {
	rrr bool
}

func (e *Echo) OnAccept(pcb *lwipcs.PCB) {
	// fmt.Printf("accept from %v\n", pcb.RemoteAddr())
	go io.Copy(pcb, pcb)
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
	pcb.Write(data)
}

func init() {
	lwipcs.Event = &Echo{}
}

//Init will init the netif
func Init(state unsafe.Pointer) unsafe.Pointer {
	return lwipcs.Init(state)
}

//Deinit will free netif
func Deinit(netif unsafe.Pointer) {
	lwipcs.Deinit(netif)
}

//Proc will proc netif input
func Proc(netif unsafe.Pointer) {
	lwipcs.Proc(netif)
}

//Stop running proc
func Stop() {
	lwipcs.Stop()
}
