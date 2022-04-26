问题描述

多线程中，clone 出的 logger 对象出现崩溃

环境
```buildoutcfg
root@debian:~/work/structlog_thread/structlog/build# cat /proc/version
Linux version 4.19.0-13-amd64 (debian-kernel@lists.debian.org) (gcc version 8.3.0 (Debian 8.3.0-6)) #1 SMP Debian 4.19.160-2 (2020-11-28)
```



```buildoutcfg
mkdir build

cd build

cmake ..

cmake --build .

watch -n 1 ./demo


```

报错信息
```buildoutcfg
root@debian:~/work/structlog_thread/structlog/build# ./demo 
free(): invalid size
Aborted
```

```buildoutcfg
root@debian:~/work/structlog_thread/structlog/build# ./demo 
{"level":"info","msg":"halo","time":"2022-04-26T12:59:39.014038213+08:00"}
{"level":"info","msg":"halo","time":"2022-04-26T12:59:39.014089286+08:00"}
{"level":"info","msg":"halo","time":"2022-04-26T12:59:39.014092789+08:00"}
{"level":"info","msg":"halo","time":"2022-04-26T12:59:39.014095594+08:00"}
{"time":"2022-04-26T12:59:39.014104669+08:00"}
{"level":"info","msg":"halo","time":"2022-04-26T12:59:39.014095594+08:00"}
"thread":1,"level":"info","msg":"in thread 1.","time":"2022-04-26T12:59:39.014107489+08:00"}
terminate called after throwing an instance of 'std::bad_alloc'
  what():  std::bad_alloc
Aborted


```



