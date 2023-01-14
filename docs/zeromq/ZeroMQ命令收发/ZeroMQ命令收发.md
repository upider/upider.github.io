# ZeroMQ命令收发

---

## 概述

zmq线程间通信包括两类，一类是用于收发命令，告知对象该调用什么方法去做什么事情，命令的结构由command_t结构体确定；另一类是socket_base_t实例与session的消息通信，消息的结构由msg_t确定。命令的发送与存储是通过mailbox_t实现的，消息的发送和存储是通过pipe_t实现的。

zeromq的线程可分为两类，一类是io线程，像reaper_t、io_thread_t都属于这一类，这类线程的特点就是内含一个轮询器poller及mailbox_t，通过poller可以监听激活mailbox_t的信号 ；另一类是zmq的socket,所有socket_base_t实例化的对象都可以看做一个单独的线程，这类线程不含poller，但同样含有一个mailbox_t，可以用于收发命令，由于不含poller，只能在每次使用socket_base_t实例的时候先处理一下mailbox_t，看是否有命令需要处理。两类线程发送命令的方式是一致的。

## 命令

```cpp
//  This structure defines the commands that can be sent between threads.
    struct command_t
    {
        //  Object to process the command.
        zmq::object_t *destination;

        enum type_t
        {
           ...
        } type;

        union {
           ...
        } args;
    };
```

对象、object_t、poller、线程、mailbox_t、命令是什么关系:
+ 在zmq中，每个线程都会拥有一个信箱，命令收发功能底层都是由信箱实现的
+ zmq提供了object_t类，用于使用线程信箱发送命令的功能（object_t类还有其他的功能），object_t还有处理命令的功能。
+ io线程内还有一个poller用于监听激活mailbox_t的信号，线程收到激活信号后，会去mailbox_t中读命令，然后把命令交由object_t处理
+ object_t发命令，poller监听命令到来信号告知线程收命令，交给object_t处理。无论是object_t、还是线程本身、还是poller其实都操作mailbox_t
+ object_t、poller、mailbox_t都绑定在同一个线程上

## 发命令
`一个对象想使用线程的发命令功能，其类就得继承自object_t`

```cpp
class object_t
{
public:
    object_t (zmq::ctx_t *ctx_, uint32_t tid_);
    void process_command (zmq::command_t &cmd_);
    ...
protected:
    ...
private:
    zmq::ctx_t *ctx;//  Context provides access to the global state.
    uint32_t tid;//  Thread ID of the thread the object belongs to.
    void send_command (command_t &cmd_);
}
```

+ object_t内含一个tid，含义就是，该object_t对象要使用哪个线程的mailbox_t
+ zmq::ctx_t，在zmq中被称为上下文语境，上下文语境简单来说就是zmq的存活环境，里面存储是一些全局对象，zmq中所有的线程都可以使用这些对象
+ zmq线程中的mailbox_t对象会被zmq存储在ctx_t对象中
+ zmq的做法就是，在上下文语境中使用一个容器slots装载线程的mailbox，在新建线程的时候，给线程分配一个线程标志tid和mailbox，把mailbox放入容器的tid那个位置，代码来说就是slots[tid]=mailbox。有了这个基础，线程A给线程B发命令就只要往slots[B.tid]写入命令就可以了

```cpp
void zmq::object_t::send_command (command_t &cmd_)
{
    ctx->send_command (cmd_.destination->get_tid (), cmd_);
}
void zmq::ctx_t::send_command (uint32_t tid_, const command_t &command_)
{
    slots [tid_]->send (command_);
}
void zmq::mailbox_t::send (const command_t &cmd_)
{
    sync.lock();
    cpipe.write (cmd_, false);
    bool ok = cpipe.flush ();
    sync.unlock ();
    if (!ok)
        signaler.send ();
}
```

## 收命令
### 1. `每个io线程都含有一个poller，io线程的结构如下`

```c++
class io_thread_t : public object_t, public i_poll_events
{
public:
    io_thread_t (zmq::ctx_t *ctx_, uint32_t tid_);
    ~io_thread_t ();
    void start (); //  Launch the physical thread.
    void stop ();//  Ask underlying thread to stop.
    ...
private:
    mailbox_t mailbox;//  I/O thread accesses incoming commands via this mailbox.
    poller_t::handle_t mailbox_handle;//  Handle associated with mailbox' file descriptor.
    poller_t *poller;//  I/O multiplexing is performed using a poller object.
}

zmq::io_thread_t::io_thread_t (ctx_t *ctx_, uint32_t tid_) :
    object_t (ctx_, tid_)
{
    poller = new (std::nothrow) poller_t;
    alloc_assert (poller);

    mailbox_handle = poller->add_fd (mailbox.get_fd (), this);
    poller->set_pollin (mailbox_handle);
}
```

### 2. `构造函数中把mailbox_t句柄加入poller中，让poller监听其读事件，所以，如果有信号发过来，poller会被唤醒，并调用io_thread_t的in_event`

```cpp
void zmq::io_thread_t::in_event ()
{
    //  TODO: Do we want to limit number of commands I/O thread can
    //  process in a single go?

    command_t cmd;
    int rc = mailbox.recv (&cmd, 0);

    while (rc == 0 || errno == EINTR) {//如果读管道中有内容或者等待信号的时候被中断，将一直读取
        if (rc == 0)
            cmd.destination->process_command (cmd);
        rc = mailbox.recv (&cmd, 0);
    }

    errno_assert (rc != 0 && errno == EAGAIN);
}
```

## socket_base_t线程收命令

### 1. `socket_base_t的每个实例都可以看成一个zmq线程，但是比较特殊，并没有使用poller，而是在使用到socket的下面几个方法的时候去检查是否有未处理的命令`

```cpp
int zmq::socket_base_t::getsockopt (int option_, void *optval_,size_t *optvallen_)
int zmq::socket_base_t::bind (const char *addr_)
int zmq::socket_base_t::connect (const char *addr_)
int zmq::socket_base_t::term_endpoint (const char *addr_)
int zmq::socket_base_t::send (msg_t *msg_, int flags_)
int zmq::socket_base_t::recv (msg_t *msg_, int flags_)
void zmq::socket_base_t::in_event ()//这个函数只有在销毁socke的时候会被用到，在后面讲zmq_close的时候会说到
```

### 2. `检查的手段就是调用process_commands方法`

```cpp
int zmq::socket_base_t::process_commands (int timeout_, bool throttle_)
{
    int rc;
    command_t cmd;
    if (timeout_ != 0) {
        //  If we are asked to wait, simply ask mailbox to wait.
        rc = mailbox.recv (&cmd, timeout_);
    }
    else {
        some code
        rc = mailbox.recv (&cmd, 0);
    }
    //  Process all available commands.
    while (rc == 0) {
        cmd.destination->process_command (cmd);
        rc = mailbox.recv (&cmd, 0);
    }
    some code
}
```

## mailbox_t

### `线程间收发命令都是通过mailbox_t实现的，现在就来看看mailbox_t到底是如何实现的，mailbox_t的声明如下`

```cpp
class mailbox_t
{
public:
    mailbox_t ();
    ~mailbox_t ();
    fd_t get_fd ();
    void send (const command_t &cmd_);
    int recv (command_t *cmd_, int timeout_);
private:
    typedef ypipe_t <command_t, command_pipe_granularity> cpipe_t; //  The pipe to store actual commands.
    cpipe_t cpipe;
    signaler_t signaler;//  Signaler to pass signals from writer thread to reader thread.

    //  There's only one thread receiving from the mailbox, but there
    //  is arbitrary number of threads sending. Given that ypipe requires
    //  synchronised access on both of its endpoints, we have to synchronise
    //  the sending side.
    mutex_t sync;//只有一个线程从mailbox中接受消息，但是会有大量的线程往mailbox中发送消息，鉴于ypipe需要同步访问两端的两端，我们必须同步发送端
    bool active; //  True if the underlying pipe is active, ie. when we are allowed to read commands from it.

    //  Disable copying of mailbox_t object.
    mailbox_t (const mailbox_t&);
    const mailbox_t &operator = (const mailbox_t&);
};
```

### `mailbox_t中的有几个属性很关键，有必要说一下`

+ cpipe，后面可能会称之为管道，ypipe_t类型，在zmq的实现中ypipe_t是一个单生产者单消费者无锁队列（下一篇会详细介绍），只有一个读命令线程和一个写命令线程的时候是线程安全的。ypipe_t的安全性谁使用谁负责。命令都是存储在cpipe中的。
+ sync，由于mailbox_t底层使用的是ypipe_t，而且多个线程向一个线程发命令的场景是很常见的，所以要互斥ypipe_t的发送端。
+ signaler，通知命令接受方，现在信箱mailbox中有命令了，你可以去读了，从代码的角度就是通知接受方mailbox_t把active设置为true。signaler的底层根据不同平台有不同实现，本质上可以看成一个socketpair，这个东西比较重要，应该先man一下，我这里不多说。
+ active，管道中是否有命令可读

### `线程th1如何向线程th2发送命令`
th1先把命令写入th2的管道cpipe中，然后刷新th2的管道，再使用signaler发送一个信号给th2，告诉th2我向你的管道写了一个命令，你可以去管道读命令了

```cpp
void zmq::mailbox_t::send (const command_t &cmd_)
{
    sync.lock();//互斥写命令端    //关于cpipe的详细实现，会在下一篇详细的介绍，现在只需要知道函数的功能就可以了
    cpipe.write (cmd_, false);//向接受送方mailbox_t管道写入命令，在没有调用flush之前，接收方看不到这个命令
    bool ok = cpipe.flush ();//刷新管道，这个时候接收方能看到刚才那条命令了
    sync.unlock ();
    if (!ok)
        signaler.send ();//发送信号给接受命令的一方
}
```

### `th2读命令`

1. 如果th2是socket_base_t实例线程，先调用process_commands,process_commands会调用循环调用mailbox_t的recv函数，直到没命令可读退出循环
2. 如果th2是io_thread_t这类线程，会有poller监听信号的到来，然后调用线程的in_event，in_event又会循环调用mailbox_t的recv函数，直到没命令可读退出循环，并睡眠，等待再次被信号唤醒
3. 需要注意的是，这两类线程对发送过的信号都在mailbox_t的recv函数中处理的

```cpp
int zmq::mailbox_t::recv (command_t *cmd_, int timeout_)
{
    //  Try to get the command straight away.
    if (active) {//开始的时候，信箱是未激活状态
        bool ok = cpipe.read (cmd_);
        if (ok)
            return 0;

        //  If there are no more commands available, switch into passive state.
        //  没有命令可读时，先把信箱设置为未激活状态，表示没命令可读，然后把对方发过来的激活信箱的信号处理一下（没什么特殊的处理，就是接受一下）
        active = false;
        signaler.recv ();
    }

    //  Wait for signal from the command sender.
    int rc = signaler.wait (timeout_);//signaler.wait的返回值有三种①wait函数出错，返回-1，并且设置errno=EINTR②返回-1并且errno=EAGAIN，表示信号没等到③等到信号。
    if (rc != 0 && (errno == EAGAIN || errno == EINTR))//这里对应wait的前两种情况
        return -1;

    //  We've got the signal. Now we can switch into active state.
    active = true;//等到激活信箱的信号了，激活信箱

    //  Get a command.
    errno_assert (rc == 0);
    bool ok = cpipe.read (cmd_);
    zmq_assert (ok);
    return 0;
}
```

active和signaler是这样合作的：写命令线程每写一条命令，先去检查读命令线程是否阻塞，如果阻塞，会调用读命令线程mailbox_t中的signaler，发送一个激活读线程mailbox_t的信号，读线程收到这个命令后在recv函数中把activ设置为true，这时，读线程循环调用recv的时候，发现active为true，就会一直读命令，直到没命令可读时，又把active设置为false，等待下一次信号到来。

### `active是否多余`
先试想一下如果不使用active，每写一条命令都必须发送一个信号读读线程，在大并发的情况下，这也是一笔消耗。而使用active，只需要在读线程睡眠的时候（没有命令可读时，io_thread_t这类线程会睡眠，socket_base_t实例线程特殊，不会睡眠）发送信号唤醒读线程就可以，可以节省大量的资源