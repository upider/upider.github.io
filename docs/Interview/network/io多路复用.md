# io多路复用（select、poll、epoll之间的区别）

## select

1. 维护了一个文件描述符（fd）数组，大小为1024
2. select方法被调用，首先需要将fd_set从用户空间拷贝到内核空间
3. select方法返回后，需要轮询fd_set

## poll

poll则是维护了一个链表，所以从理论上，poll方法中，单个进程能监听的fd不再有数量限制。但是轮询，复制等select存在的问题，poll依然存在

## epoll

没有文件描述符数量限制，不需要遍历fd，不用拷贝fd

并不是所有的情况中epoll都是最好的，比如当fd数量比较小的时候，epoll不见得就一定比select和poll好