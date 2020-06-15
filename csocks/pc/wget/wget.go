package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
)

func main() {
	req, err := http.NewRequest("GET", os.Args[1], nil)
	if err != nil {
		fmt.Printf("error:%v", err)
		return
	}
	res, err := http.DefaultClient.Do(req)
	if err != nil {
		fmt.Printf("error:%v", err)
		return
	}
	defer res.Body.Close()
	data, err := ioutil.ReadAll(res.Body)
	if err != nil {
		fmt.Printf("error:%v", err)
		return
	}
	fmt.Printf("%v\n", string(data))
}
