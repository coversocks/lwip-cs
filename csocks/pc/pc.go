package main

import "C"

import (
	"github.com/coversocks/lwipcs/csocks"
)

func main() {

}

//export go_cs_proc
func go_cs_proc() {
	go go_cs_read()
	csocks.Proc()
}

//export go_cs_read
func go_cs_read() {
	csocks.Read()
}
