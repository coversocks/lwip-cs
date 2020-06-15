package csocks

import (
	"fmt"
	"regexp"
	"strings"
	"testing"
)

func TestSocks(t *testing.T) {

}

func TestPattern(t *testing.T) {
	domain := "www.google.com"
	parts := strings.SplitAfterN(domain, ".", 3)
	for i, p := range parts {
		parts[i] = strings.Trim(p, ".")
	}
	fmt.Println(parts)
	if len(parts) == 3 {
		parts = parts[1:]
	}
	pattern := regexp.MustCompile(fmt.Sprintf("[\\|\\.]*%v\\.%v$", parts[0], parts[1]))
	fmt.Println(pattern.MatchString("||google.com"))
}
