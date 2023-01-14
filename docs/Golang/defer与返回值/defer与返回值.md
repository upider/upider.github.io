# defer与返回值

## 例子
```go
package main

import "fmt"

func f1() int {
	var x int
	//无法修改x值
	defer func() {
		x++
	}()
	//无法修改x值
	defer func(r *int) {
		*r++
	}(&x)
	return x
}

func f2() (x int) {
	//这样才能捕获x，并修改x值
	defer func() {
		x++
	}()
	//这样才能捕获x，并修改x值
	defer func(r *int) {
		*r++
	}(&x)
	return
}

func main() {
	fmt.Println(f1())
	fmt.Println(f2())
	fmt.Println("Hello, World!")
}
```

## 解释
return其实应该包含前后两个步骤
1. 第一步是给返回值赋值（若为有名返回值则直接赋值，若为匿名返回值则先声明再赋值）
2. 第二步是调用RET返回指令并传入返回值，而RET则会检查defer是否存在，若存在就先逆序插播defer语句，最后RET携带返回值退出函数

所以，如果是匿名返回值，那么defer中的变量与返回值根本不是同一个值，因为返回值在return时才生成