# Cache lab writeup

## Part A

仿造`csim-ref`，写一个cache模拟器，模拟在一系列的数据访问中cache的命中、不命中与牺牲行的情况，其中，需要牺牲行时，用LRU替换策略进行替换。

Cache主体的数据结构如下

```c
typedef long long unsigned mem_addr_t;
struct cache_line_t{
    mem_addr_t tag;
    int valid;
    unsigned int lru; 
};
typedef struct cache_line_t* cache_set_t;
typedef cache_set_t* cache_t;

cache_t cache;
```

每次获取数据时，都需要修改该数据中的LRU。同时，如果该数据并没有存放于Cache中，则需要根据LRU来驱逐某条Cache_line。



## Part B

写一个实现矩阵转置的函数，并且使函数调用过程中对cache的不命中数miss尽可能少。

测试程序所使用的cache模拟器的参数为`-S 5 -E 1 -B 5`。即该cache为内含32个缓存行的*直接映射高速缓存*，其中每个缓存行可以存放32位数据，即8个int型数据。该cache最多可读入32x32矩阵中的8行数据。故我们可将32x32矩阵以每块8x8的大小分割并转置存放到另一个矩阵中，这样便可以减小cache的miss数。

