# ShellLab writeup

任务就是补全tsh.c的代码，具体为以下函数：

• eval: Main routine that parses and interprets the command line. [70 lines]
• builtin cmd: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs. [25 lines]
• do bgfg: Implements the bg and fg built-in commands. [50 lines]
• waitfg: Waits for a foreground job to complete. [20 lines]
• sigchld handler: Catches SIGCHILD signals. 80 lines]
• sigint handler: Catches SIGINT (ctrl-c) signals. [15 lines]
• sigtstp handler: Catches SIGTSTP (ctrl-z) signals. [15 lines]



既然是shell，就会用到fork。

## Notes

- fg和bg命令的前提是先crtl-z去挂起，再转移。实现就是修改job的state，然后通过SIGCONT去resume.

  – The bg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in the background. The <job> argument can be either a PID or a JID.
  – The fg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in the foreground. The <job> argument can be either a PID or a JID.

- 后台进程是不受crtl-c，crtl-z作用的。那么这需要gid与tsh的不同，不然kernel会把signal发给shell整个group。       真实情况下键入crtl-c等，是bash去处理，去kill发生中断的。shell实际上有可能是getchar的，那么接收到特殊字符就会进行处理。shell给前台进程组发信号。	可见[linux 终端下敲ctrl-c时，到底发生了什么?(转)](https://www.cnblogs.com/softidea/p/4986369.html)
  
- 解决方法就是调用`setpgid(0,0)`，这将使子进程放入一个新的进程组中，其中该进程组的ID与子进程的PID相同。这确保bash前台进程组中只有一个进程，即tsh进程。
  
- 当我们在真正的shell（例如bash）中执行tsh时，tsh本身也是被放在前台进程组中的，它的子进程也会在前台进程组中，例如下图所示：

```
               +----------+
               |   Bash   |
               +----+-----+
                    |
+-----------------------------------------+
|                   v                     |
|              +----+-----+   foreground  |
|              |   tsh    |   group       |
|              +----+-----+               |
|                   |                     |
|         +--------------------+          |
|         |         |          |          |
|         v         v          v          |
|       /bin/ls    /bin/sleep  xxxxx      |
|                                         |
|                                         |
+-----------------------------------------+
```

所以当我们在终端输入`ctrl-c` (`ctrl-z`)的时候，`SIGINT` (`SIGTSTP`)信号应该被送给每一个在前台进程组中的**所有进程**，包括我们在tsh中认为是后台进程的程序。一个决绝的方法就是在`fork`之后`execve`之前，子进程应该调用`setpgid(0, 0)`使得它进入一个新的进程组（其pgid等于该进程的pid）。tsh接收到`SIGINT` `SIGTSTP`信号后应该将它们发送给tsh眼中正确的“前台进程组”（包括其中的所有进程）。

- 如果子进程在tsh`fork`之后、`addjob`前结束进程，则此时会因为`SIGCHLD`信号，转去信号处理程序里执行`deletejob`。此时的执行顺序就变成了`deletejob`->`addjob`，这将会产生一个永远存在的job，即便该job所指定的进程已经终止了。所以我们在执行`fork`函数前，将一些可能会导致**条件竞争**的信号阻塞，待`addjob`执行完成后再来处理信号。



