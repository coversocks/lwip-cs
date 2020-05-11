package main

import "C"

import (
	"unsafe"

	"github.com/coversocks/lwipcs/csocks"
)

func main() {

}

//export go_cs_init
func go_cs_init(state unsafe.Pointer) unsafe.Pointer {
	return csocks.Init(state)
}

//export go_cs_deinit
func go_cs_deinit(netif unsafe.Pointer) {
	csocks.Deinit(netif)
}

//export go_cs_proc
func go_cs_proc(netif unsafe.Pointer) {
	csocks.Proc(netif)
}

//export go_cs_stop
func go_cs_stop() {
	csocks.Stop()
}
