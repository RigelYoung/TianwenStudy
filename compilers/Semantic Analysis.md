# Semantic Analysis

## Introduction

![image-20210207223630558](Semantic%20Analysis.assets/image-20210207223630558.png)

![image-20210207223742324](Semantic%20Analysis.assets/image-20210207223742324.png)

![image-20210207223958175](Semantic%20Analysis.assets/image-20210207223958175.png)

语法分析只是去分析语法，提取出ast语法树，只追求语法正确。

​	而semantic具体到语义，检查具体的使用。如变量有没有定义。



## Scope

![image-20210207224620153](Semantic%20Analysis.assets/image-20210207224620153.png)

作用域scope的作用就是将变量uses匹配到对应的声明/定义。	不同作用域就不能匹配。



![image-20210207225310484](Semantic%20Analysis.assets/image-20210207225310484.png)



![image-20210207225401682](Semantic%20Analysis.assets/image-20210207225401682.png)

大部分语言都是static scope静态作用域，即scope只依赖于程序代码，是直接确定的，而不是运行时确定的。



![image-20210207225910200](Semantic%20Analysis.assets/image-20210207225910200.png)

有些语言是符合就近原则的，即uses匹配到上面最近的一个declaration。因此如果变量是嵌入的（如嵌入的作用域中），uses就匹配到相同作用域下的declaration。



![image-20210207230410994](Semantic%20Analysis.assets/image-20210207230410994.png)

method方法的情况就更加复杂。继承，覆盖等



## Symbol Tables

![image-20210207233957767](Semantic%20Analysis.assets/image-20210207233957767.png)

语义分析就在ast上进行的（其实实习在做的规范检查就属于语义分析），访问ast结点。

![image-20210207234338046](Semantic%20Analysis.assets/image-20210207234338046.png)

正如ASTVisitor，访问子树中每个ast结点，既有visit函数，又有leave函数去进行清理，恢复初始状态。

![image-20210207235022989](Semantic%20Analysis.assets/image-20210207235022989.png)

分析少不了对子树进行递归遍历。因为树的访问形式就是递归。

​	**符号表symbol table就是去记录当前标识符的binding**



![image-20210207235655875](Semantic%20Analysis.assets/image-20210207235655875.png)

find就是找到在stack top的binding，栈底的binding被掩盖。	“就近”



![image-20210208000225801](Semantic%20Analysis.assets/image-20210208000225801.png)

对于let这种，stack就可以用于symbol table。leave子树（作用域）时，就正好以add的倒序去remove symbol，栈满足这个需求。

但是stack不能应对复杂的情况（或者说是去识别illegal的情况），两个x的定义在同一个域，这是illegal的，但是用stack的话，add第二个x后，就会覆盖第一个x，从而检查不出multi-definition错误。



![image-20210208001222844](Semantic%20Analysis.assets/image-20210208001222844.png)

所以一个symbol table需要具备上面5个功能。

​	check_scope可用于检查上面的单scope中的multi-definition.



![image-20210208001704607](Semantic%20Analysis.assets/image-20210208001704607.png)

像对于类这样的，类名可在被定义前使用，所以我们检查时，就有可能需要遍历两次ast甚至更多次。因为定义在后面，第一遍拿到定义后，第二次遍历时才能去判断使用情况。   如果嵌套的话，就会需要更多次。			向前引用都这样。



## Types

![image-20210208002628330](Semantic%20Analysis.assets/image-20210208002628330.png)

类型Type不仅仅包括数据集合，还包括对数据的操作！！

![image-20210208003010632](Semantic%20Analysis.assets/image-20210208003010632.png)

对于不同类型，一个操作的汇编实现有时是一样的。比如mov



![image-20210208003433830](Semantic%20Analysis.assets/image-20210208003433830.png)

**type checking就是去检查各个数据的类型是否与operation匹配。	我们认为operation就是正确的真实目的，所以是检查数据类型是否正确。**



![image-20210208003822791](Semantic%20Analysis.assets/image-20210208003822791.png)



![image-20210208005059775](Semantic%20Analysis.assets/image-20210208005059775.png)

动态类型更灵活，如python，但是运行时有overhead，并且不能提前检测出可能错误。

​	另外，动态类型和前一节的动态scope完全是两个概念，没有任何关系。

![image-20210208005334697](Semantic%20Analysis.assets/image-20210208005334697.png)



![image-20210208005714094](Semantic%20Analysis.assets/image-20210208005714094.png)



## Type Checking

首先要明确，type checking只是语义分析的一部分，并不是全部。

![image-20210212212204832](Semantic%20Analysis.assets/image-20210212212204832.png)

type checking用的形式是逻辑推断规则。

![image-20210212212940115](Semantic%20Analysis.assets/image-20210212212940115.png)

逻辑推断规则与if then一致。



![image-20210212213355250](Semantic%20Analysis.assets/image-20210212213355250.png)

![image-20210212213508593](Semantic%20Analysis.assets/image-20210212213508593.png)



![image-20210212215014091](Semantic%20Analysis.assets/image-20210212215014091.png)

provable就是可论证成立。

![image-20210212215336616](Semantic%20Analysis.assets/image-20210212215336616.png)

![image-20210212215546972](Semantic%20Analysis.assets/image-20210212215546972.png)

就是根据这些rules去推导expression的结果类型。



![image-20210212214641533](Semantic%20Analysis.assets/image-20210212214641533.png)

sound就是安全子集，保证任何情况都能推导正确。

​	但是能具体还是要具体。比如java类全推导成Object是sound，但是没有意义了。



![image-20210212220907458](Semantic%20Analysis.assets/image-20210212220907458.png)

**rules的形式化结构就是AST的一个子树！！  conclusion是子树的root，每个hypothesis就是叶结点。那么type checking检查时，也是bottom-up去检查。**



## Type Environments

![image-20210212221811329](Semantic%20Analysis.assets/image-20210212221811329.png)

![image-20210212222033364](Semantic%20Analysis.assets/image-20210212222033364.png)

**type environment就是一个可以给出指定free变量（即在该表达式外定义的变量）的type的函数。**

​	普通rules的一个问题就是，不知道基础条件，如变量x的type。只有知道x的type，才能利用rules去推断x+y的type。



![image-20210212230804407](Semantic%20Analysis.assets/image-20210212230804407.png)

![image-20210212231110810](Semantic%20Analysis.assets/image-20210212231110810.png)

![image-20210212232530230](Semantic%20Analysis.assets/image-20210212232530230.png)

![image-20210212233636372](Semantic%20Analysis.assets/image-20210212233636372.png)



**type environment就是由symbol table实现的**。 O[T/X]就表示，在检查e1之前，需要调整O去包含x:T0。 当leave 检查e1时，需要把x:T0从O中remove。



![image-20210212234608692](Semantic%20Analysis.assets/image-20210212234608692.png)

type environment是从root到leaf传递的，是因为正常语义检查ast时，也是top-down。





## Subtyping

subtype就是继承关系这样的，子类。

![image-20210213104345978](Semantic%20Analysis.assets/image-20210213104345978.png)

这里let x:T就是向上转型				解决subtyping就是引入了新规则去描述subtype

![image-20210213105609784](Semantic%20Analysis.assets/image-20210213105609784.png)



![image-20210213105723097](Semantic%20Analysis.assets/image-20210213105723097.png)

当一个变量有2种可能的类型时，利用向上转型去统一两种情况

![image-20210213110042449](Semantic%20Analysis.assets/image-20210213110042449.png)

可以通过查类的继承树去获取两个类的最小基类（java是单根继承，所以最差的情况就是Object）



![image-20210213110322228](Semantic%20Analysis.assets/image-20210213110322228.png)

![image-20210213110519786](Semantic%20Analysis.assets/image-20210213110519786.png)



## Typing Methods

这里是检查methods和method calls

![image-20210213111556648](Semantic%20Analysis.assets/image-20210213111556648.png)

![image-20210213231211193](Semantic%20Analysis.assets/image-20210213231211193.png)

![image-20210213232357162](Semantic%20Analysis.assets/image-20210213232357162.png)



![image-20210213232816597](Semantic%20Analysis.assets/image-20210213232816597.png)

![image-20210214000055162](Semantic%20Analysis.assets/image-20210214000055162.png)

self_type就如this关键字

​	要注意，每个id都有自己的一个symbol table。

![image-20210214000450884](Semantic%20Analysis.assets/image-20210214000450884.png)

![image-20210214001630763](Semantic%20Analysis.assets/image-20210214001630763.png)

每种语言的rules模型都有一些差异。



## Implementing Type Checking

![image-20210214004411316](Semantic%20Analysis.assets/image-20210214004411316.png)

COOL语言只用遍历一遍AST就能完成type checking。

​	首先要明确，检查的顺序是bottom-up的。  首先到达一个结点时，visit来更新type environment，之后进行type check（对子树遍历），结束后leave去remove type environment。



![image-20210214003122219](Semantic%20Analysis.assets/image-20210214003122219.png)

![image-20210214003422826](Semantic%20Analysis.assets/image-20210214003422826.png)

T0，T1并不是确定的一个类型，只是指代一个类型，像变量一样，去描述type规则。 这里就是判断e0得到的type T0是否为e1的type T1的subtype。e0可以是int，也可以是float，都适用于该rule.



