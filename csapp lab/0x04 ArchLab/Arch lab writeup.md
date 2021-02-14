# Arch lab writeup

## Part A

用Y86指令模拟example.c文件中的三个函数的功能。

1. 对下面的链接Sample linked list数据进行操作，实现sum_list（）函数的功能

```assembly
# Sample linked list  实验数据
        .align 4
        ele1:
                .long 0x00a
                .long ele2
                .long 0x0b0
                .long ele3
        ele3:
                .long 0
        ele2:
                .long 0xc00

        /* linked list element */ 链表的定义
        typedef struct ELE {
            int val;
            struct ELE *next;
        } *list_ptr;
        

# 函数执行开始地址为0
        .pos    0
 init:  irmovl  Stack, %esp
        irmovl  Stack, %ebp
        call    Main
        halt

 # Sample linked list 函数操作中需要运用到的数据定义
        .align  4
 ele1:  
        .long   0x00a
        .long   ele2
 ele2:  
        .long   0x0b0
        .long   ele3
ele3:   
        .long    0xc00
        .long   0

#定义Main函数，调用sum_list函数
Main:   pushl   %ebp
        rrmovl  %esp,   %ebp
        irmovl  ele1 ,  %eax
        pushl   %eax
        call    sumlist
        rrmovl  %ebp , %esp
        popl    %ebp
        ret

# int sum_list(list_ptr ls) 相关sum_list 函数的实现
sumlist:    
        pushl   %ebp
        rrmovl  %esp ,%ebp
        xorl    %eax,%eax       #the return val  = 0
        mrmovl  8(%ebp) , %edx
        andl    %edx , %edx     #ls == 0 ?
        je  End                          
Loop:   mrmovl  (%edx) , %ecx       #ls->val  ==> %ecx
        addl    %ecx , %eax         #val += ls->val
        irmovl  $4 , %edi      
        addl    %edi , %edx         #next ==> edx
        mrmovl (%edx),  %esi        
        rrmovl  %esi , %edx         #ls->next ==>edx
        andl    %edx , %edx         #set condition codes
        jne Loop                    #if ls != 0 goto Loop

End:    rrmovl  %ebp , %esp
        popl    %ebp
        ret     

#定义栈的起始地址
        .pos 0x100
Stack:
```



2.模拟example函数里面的rsum_list函数，是sum_list函数的递归版本，初始，数据，Mian函数和栈代码与sum_list的均相同。

```assembly
rsum_list:
        pushl   %ebp
        rrmovl  %esp , %ebp
        pushl   %ebx
        irmovl  $4 , %esi
        subl    %esi , %esp
        xorl    %eax , %eax
        mrmovl  8(%ebp),%edx
        andl    %edx , %edx
        je  End
        mrmovl (%edx) , %ebx
        irmovl  $4 , %esi
        addl    %esi , %edx
        mrmovl (%edx) , %edi
        rmmovl  %edi , (%esp)
        call    rsum_list
        addl    %ebx , %eax

    End:    
        addl    %esi , %esp
        popl    %ebx
        popl    %ebp
```

3.copy_block函数，拷贝源地址数据到目标地址，并且计算所有数据Xor值。

```assembly
    .pos    0
init:   irmovl  Stack,  %esp
        irmovl  Stack,  %ebp
        call    Main
        halt

    .align 4
# Source block
src:
        .long 0x00a
        .long 0x0b0
         .long 0xc00
# Destination block
dest:
         .long 0x111
         .long 0x222
        .long 0x333

Main:   pushl   %ebp
        rrmovl  %esp , %ebp
        irmovl  $12 , %esi
        subl    %esi , %esp
        irmovl  src , %eax
        rmmovl  %eax , (%esp)
        irmovl  dest , %eax
        rmmovl %eax , 4(%esp)
        irmovl  $3, %eax
        rmmovl %eax, 8(%esp)
        call    copy_block
        irmovl  $12 , %esi
        addl    %esi , %esp 
        popl    %ebp
        ret 

copy_block:
        pushl   %ebp
        rrmovl  %esp, %ebp
        xorl    %eax , %eax

        mrmovl  12(%ebp) , %edx     #edx <==>dest
        mrmovl  8(%ebp) , %esi      #esi <==> src
        mrmovl  16(%ebp),%ecx       #ecx <==> len
        andl    %ecx, %ecx
        je  End
Loop:   mrmovl   (%esi) , %ebx
        rmmovl  %ebx , (%edx)       #copy src value to dest
        xorl    %ebx , %eax         #compute the value ^= val
        irmovl  $4 ,  %edi      
        addl    %edi , %edx         #dest++
        addl    %edi , %esi         #src++
        irmovl  $1,%edi             
        subl    %edi , %ecx         #len--
        jne Loop

End:    popl    %ebp
        ret

        .pos    0x100
 Stack:
```



## Part B

为SEQ处理器添加指令`iaddq`，所要修改的文件为`seq-full.hcl`，其工作目录为`arch-lab/sim/seq`。由于`iaddq`指令既与运算操作相关，又与立即数处理相关，所以指令的功能添加可以参考`seq-full.hcl`中的`IOPQ`以及`IIRMOVQ`。



## Part C

修改ncopy.ys和pipe-full.hcl 文件使得ncopy.ys 的运行速度越快越好

该部分实验最主要实现的是优化ncopy.ys函数。本解法主要涉及到的是CSAPP第五章的优化方法中的“循环展开方法”和第4章中的“加载使用冒险”

对于ncopy.ys中的ncopy函数进行了4次的循环展开。并且在原始的函数中存在`加载使用冒险`，如下所示，mrmovl从存储器中读入src到esi，rmmovl从esi存储到dest中，期间因为加载使用冒险所以需要暂停一个周期，针对这个进行改进。

```
mrmovl  (%ebx), %esi   # read val from src
rmmovl %esi, (%ecx)   # store src[0] to dest[0]12
```

主要改进的方法在这两条指令中插入另一条mrmovl指令，避免了冒险节省时间，也为后面的循环展开提前获取到了值。

ncopy.ys相应改进部分实现如下所示：

```assembly
# You can modify this portion
# Loop Header
        xorl    %eax , %eax
        iaddl   $-4 , %edx #len = len -4
        andl    %edx ,  %edx    
        jl  remian
Loop:   mrmovl (%ebx) , %esi
        mrmovl 4(%ebx),%edi
        rmmovl %esi , (%ecx)
        andl    %esi ,%esi
        jle LNpos1
        iaddl   $1 , %eax
LNpos1: rmmovl %edi , 4(%ecx)
        andl    %edi , %edi
        jle     LNpos2
        iaddl   $1, %eax
LNpos2:mrmovl 8(%ebx) , %esi
        mrmovl 12(%ebx),%edi
        rmmovl %esi ,8 (%ecx)
        andl    %esi ,%esi
        jle LNpos3
        iaddl   $1 , %eax
LNpos3: rmmovl %edi , 12(%ecx)
        andl    %edi , %edi
        jle     nextLoop
        iaddl   $1, %eax
nextLoop:
        iaddl   $16,%ebx
        iaddl   $16,%ecx
        iaddl   $-4,%edx
        jge Loop            

# maybe just remain less than 3
remian:  iaddl  $4 , %edx  # Restore the true len
        iaddl   $-1, %edx
        jl  Done
        mrmovl (%ebx) , %esi
        mrmovl 4(%ebx),%edi
        rmmovl %esi , (%ecx)
        andl    %esi ,%esi
        jle rNpos
        iaddl   $1 , %eax
rNpos:  
        iaddl   $-1, %edx
        jl  Done
        rmmovl  %edi , 4(%ecx)
        andl    %edi , %edi
        jle     rNpos1
        iaddl   $1, %eax
rNpos1:
        iaddl   $-1 , %edx 
        jl  Done
        mrmovl 8(%ebx) , %esi
        rmmovl %esi , 8(%ecx)
        andl    %esi ,%esi
        jle Done
        iaddl   $1 , %eax
```



