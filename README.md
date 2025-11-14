****
# libcc

![platform](https://img.shields.io/badge/platform-linux%20%7C%20windows%20%7C%20macos-blue)
<br>

`libcc`是跨平台，多线程，轻量级的C语言库，提供了更简单的接口和更丰富的协议。提供的函数功能包括：字符串、日志、双向/单向链表、哈希表、网络通信、JSON、XML、INI配置文件读取、AES、DES、MD2、MD4、MD5、base16/base58/base64编码/解码、url编码/解码、时间轮计时器等等。详细信息请参阅C文件

针对各个平台，封装了统一的接口，简化了各类开发过程中常用操作，使你在开发过程中，更加关注实际应用的开发，而不是把时间浪费在琐碎的接口兼容性上面，并且充分利用了各个平台独有的一些特性进行优化。
这个项目的目的是为了使C开发更加的灵活高效。


```C
//跨平台，超轻量，易扩展，框架代码如下：
#define CALL(fn, ...) fn(__VA_ARGS__)

//使用示例：输出hello World!
CALL(printf, "Hello World!\n");
/*
 * 开一个玩笑:)！
 * 
 * 只是出于对C的热爱，累积一份属于自己的C代码
 */
```

## ✨ 特征

- 跨平台（Linux, Windows, MacOS, FreeBSD, Android, iOS）
- 提供高性能事件循环（网络IO事件、定时器事件）
- 提供io轮询器，针对epoll, poll, select, kqueue进行跨平台封装
- 提供高精度、低精度定时器
- TCP支持心跳、转发、拆包、多线程安全write和close等特性
- 提供file、directory、socket、thread、time等常用系统接口
- 提供atomic、atomic64接口
- 提供mutex、semaphore、spinlock等事件、互斥、信号量、自旋锁操作
- 提供获取函数堆栈信息的接口，方便调试和错误定位
- 提供跨平台动态库加载接口（如果系统支持的话）
- 提供 base16/base58/base64 编解码
- 提供 AES、DES、MD2、MD4、MD5、SHA 等常用hash算法
- 提供 Syslog 日志输出、断言等辅助调试工具
- 提供 URL 编解码
- 提供 JSON、XML、INI配置文件读取
- [简单实现 HTTPS 服务端/客户端模块](https://github.com/libcc/libcc/blob/2.0/tests/test_url_request.c)
- [简单实现 WebSocket 服务端模块](https://github.com/libcc/libcc/blob/2.0/tests/test_ws.c)
- 简化数据库操作接口，适配各种数据源，通过统一的url来自动连接打开支持的数据库

如果你想了解更多，请参考：[在线文档](https://libcc.cn/docs/libcc/documentation.html), [Github](https://github.com/libcc/libcc)。
## ⌛️ 安装教程

## 入门与体验

```shell
# 下载编译
git clone https://github.com/libcc/libcc.git
cd libcc

#1、通过Makefile:编译 (Linux,freeBSD,macOS)
make .so platform=linux debug=1
#或者
cd ./build
./build.sh debug
# Windows 下MSYS2环境 执行
./build.cmd debug

#2、通过Visual Studio编译 (Windows)
proj.Win/libcc.vcxproj

#3、通过Android JNI编译
#打开 .bash_profile 配置 $NDK_ROOT = (Android NDK目录)
cd proj.Android/JNI
$NDK/ndk-build NDK_DEBUG=1
#或者
./build_NDK.sh

#4、通过Xcode 编译(macOS,iOS)
proj.OSX/cc.xcodeproj
proj.IOS/cc.xcodeproj
```

## ✨ HTTP Server
[HTTP Server](https://libcc.cn/docs/libcc/tutorials/event-http.html) 基本的HTTP服务器教程将向您展示如何配置HTTP服务器，同时您将熟悉事件管理器和服务器API。
## ✨ TCP Client


## Install MySQL8 devel
  * Centos
    * wget https://repo.mysql.com//mysql80-community-release-el7-7.noarch.rpm
    * rpm -ivh mysql80-community-release-el7-7.noarch.rpm
    * yum -y install mysql-devel

## OpenSSL Download Page
  * https://slproweb.com/products/Win32OpenSSL.html

  * iOS
    * https://www.openssl.org/source/openssl-3.2.5.tar.gz
    * sudo ./Configure ios64-cross --prefix=/opt/libcc/include/openssl
    * make && make install

## SQLite Download Page
  * https://www.sqlite.org/download.html
  * download：sqlite-amalgamation-3500100.zip sqlit3 header
  * MSYS2 build sqlite3
    * gcc -shared -o sqlite3.dll sqlite3.c -Wl,--out-implib,libsqlite3.a
    * gcc -DSQLITE_ENABLE_COLUMN_METADATA sqlite3.c -shared -o sqlite3.dll -Wl,--out-implib,libsqlite3.a

## Linux Ubuntu/Debian
  * sudo apt-get install libsqlite3-dev
  * sudo apt-get install libmysqlclient-dev

## FreeBSD
  * sudo pkg install openssl
  * sudo pkg install mysql80-client
  * sudo pkg install sqlite3

## macOS Homebrew
  * brew install sqlite
  * brew install mysql-client

## Download ODBC driver for MacOSX
  * https://learn.microsoft.com/en-us/sql/connect/odbc/download-odbc-driver-for-sql-server?view=sql-server-ver17&redirectedfrom=MSDN
  * brew tap microsoft/mssql-release https://github.com/Microsoft/homebrew-mssql-release
  * brew update
  * HOMEBREW_ACCEPT_EULA=Y brew install msodbcsql18 mssql-tools18

## ⚡️ Support
Email: [libcc.cn@gmail.com](mailto:libcc.cn@gmail.com)
