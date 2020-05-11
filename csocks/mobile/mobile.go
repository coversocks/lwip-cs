package android

import (
	"unsafe"

	"github.com/coversocks/lwipcs/csocks"
)

//Init will init the netif
func Init(state unsafe.Pointer) unsafe.Pointer {
	return csocks.Init(state)
}

//Deinit will free netif
func Deinit(netif unsafe.Pointer) {
	csocks.Deinit(netif)
}

//Proc will proc netif input
func Proc(netif unsafe.Pointer) {
	csocks.Proc(netif)
}

//Stop running proc
func Stop() {
	csocks.Stop()
}
