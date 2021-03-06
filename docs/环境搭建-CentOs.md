1. 安装虚拟机
```shell
- 下载镜像
- CentOS 7.6
- CentOS-7-x86_64-Minimal-1810.iso
- 一路next即可
- 安装后查看ip地址以便于连接ssh：$ ip addr
```

2. 切换root用户
```shell
- 输入 su 时候如果有鉴定故障，则
$ sudo passwd root
输入新的密码
$ su
输入密码即可
```

3. 创建存放软件包的目录
```shell
# mkdir soft
# cd soft
```
- soft文件夹的绝对路径是 /soft。

4. 创建软件安装目录并设置环境变量
```shell
# mkdir apps
# cd apps
# mkdir bread           // bread 是用户名

# vi /etc/profile

// 然后在 /etc/profile 文件最后加上下面两句话：
export PATH=/apps/bread/bin:$PATH
export LD_LIBRARY_PATH=/apps/bread/lib:/apps/bread/lib64:$LD_LIBRARY_PATH

:wq

# source /etc/profile
```
- 注意软件会安装在 /apps/bread 目录下。
- 将要用到的程序都安装到一个自定义路径，不会与系统路径冲突。可以多版本并存。（例如覆盖系统自带的gcc版本可能会有问题）。但需要将自定义的路径加入到PATH中。(我的自定义路径是/apps/bread)

5. 安装 wget
```shell
yum -y install wget
(-y 选项是一路 yes 的意思)
```

6. 安装 ohmyzsh
```shell
百度 https://ohmyz.sh/ 找到 wget 方式的连接。

# yum install zsh
# yum install git
# sh -c "$(wget https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh -O -)"       // 国外---也是要普通用户执行，普通用户才能用。
# sh -c "$(wget https://gitee.com/mirrors/oh-my-zsh/raw/master/tools/install.sh -O -)"      // 国内---也是要普通用户执行，普通用户才能用。
# echo $SHELL       // 查看当前默认的shell

# chsh -s /bin/zsh      // 切换到zsh（注意：切换之后.bash_profile中的配置将不起作用,需要到.zshrc重新配置，或者把.bash_profile的内容复制到.zshrc里面）
```

7. 安装 Vim
```shell
# yum install gcc gcc-c++               // ./configure需要用到，安装到的是 4.8.5 版本
# yum install ncurses-devel             // vim的依赖
# yum install ctags                     // C++的一些链接提示
# yum install bzip2                     // 压缩解压缩工具
# wget https://ftp.nluug.nl/pub/vim/unix/vim-8.1.tar.bz2 --no-check-certificate
# ls -lh vim-8.1.tar.bz2                // 查看文件信息
# tar xvf vim-8.1.tar.bz2
# cd vim81
# ./configure --prefix=/apps/bread      // bread是用户名
# make -j4
# make install
# which vim

# cd /home/bread						
# git clone https://github.com/sylar-yin/myvim.git
# cd myvim
# cp .vim ~/ -rf						// 这些用普通用户的话，是拷贝到普通用户的根目录下。
# cp .vimrc ~/							// 如果用root用户操作，那么最后vim在普通用户下没有相关配置。
# find / -name ".vim"
# find / -name ".vimrc"

# vi /etc/profile

// 然后在 /etc/profile 文件最后加上下面两句话：
alias vctags="ctags -R --c++-kinds=+p --fields=+iaS --extra=+q"

:wq

# source /etc/profile
```
- 必须使用 VIM7.4 以上的版本才能正常显示C++11中的一些语法(lambda)
- 该方式很容易导致文件大小是0，从而导致解压失败，你会发现目录下有一堆.1 .2 .3...这些文件。
- ctags 生成c++ tags: ctags -R --c++-kinds=+p --fields=+iaS --extra=+q
- 为了方便使用，在profile里面 alias vctags="ctags -R --c++-kinds=+p --fields=+iaS --extra=+q"
- 当代码有新的结构，函数定义后，执行一下 vctags ，就可以了。

8. 安装bison
```shell
# yum -y install bison
```
- 没有安装bison，编译中会提示 “WARNING: ‘bison’ is missing on your system.”

9. 安装texinfo
```shell
# yum -y install texinfo

// 如果报没有这个包时，网上说可以：
# yum install -y dnf
# dnf update
# dnf --enablerepo=PowerTools install texinfo
```
- 没有安装texinfo，编译中会提示“WARNING: ‘makeinfo’ is missing on your system”
- 也可以在 http://ftp.gnu.org/gnu/texinfo/ 里面找相应的包，然后用 wget 获取安装。

10. 安装autoconf
```shell
# wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
# tar xvf autoconf-2.69.tar.gz
# cd autoconf-2.69
# ./configure --prefix=/apps/bread        // 设置安装路径
# make -j4
# make install
# which autoconf                          // 验证安装成功
```
- gcc安装需要依赖automake-1.15以上版本，automake-1.15以上版本，需要依赖autoconf 2.69

11. 安装automake
```shell
# wget http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
# tar xvf automake-1.15.tar.gz
# cd automake-1.15
# ./configure --prefix=/apps/bread

// 修改 Makefile 查找 /doc/automake-$(APIVERSION)
// doc/automake-$(APIVERSION).1: $(automake_script) lib/Automake/Config.pm
//    $(update_mans) automake-$(APIVERSION) --no-discard-stderr
// (3686行，加上--no-discard-stderr)
# make -j4
# make install
# which automake
```
- gcc安装需要依赖automake-1.15以上版本

12. GCC 安装
```shell
# wget http://ftp.tsukuba.wide.ad.jp/software/gcc/releases/gcc-9.1.0/gcc-9.1.0.tar.xz
# tar xvJf gcc-9.1.0.tar.xz               // -j是调用bzip2解缩文件
# cd gcc-9.1.0

// 报错error: Cannot download gmp-6.1.0.tar.bz2 from ftp://gcc.gnu.org/pub/gcc/infrastructure/的时候，
// vim contrib/download_prerequisites 改base_url为 https://gcc.gnu.org/pub/gcc/infrastructure/
// 如果报证书过期，可以在遍历url的时候，改成 ${base_url}${ar} --no-check-certificate，大概是222行（加了选项参数之后就不要引号了）。
# sh contrib/download_prerequisites       // 下载gcc的一些依赖（主要是gmp-6.1.0.tar.bz2 isl-0.18.tar.bz2 mpc-1.0.3.tar.gz mpfr-3.1.4.tar.bz2）

# mkdir build
# cd build
# ../configure --enable-checking=release --enable-languages=c,c++ --disable-multilib --prefix=/apps/bread
# make -j4            // make j4失败的话，可以试试make，本机-j4也要一个小时...
# make install
# which gcc
/apps/bread/bin/gcc
```
- GCC9.1版本完整支持 C++ 11，C++ 14，C++ 17，而且错误提示更友好
- GCC安装的时间会比较长，大概半小时~2小时，取决于机器性能，需要耐心等待

13. GDB安装
```shell
# wget http://ftp.gnu.org/gnu/gdb/gdb-8.3.tar.xz
# tar xvf gdb-8.3.tar.xz
# cd gdb-8.3
# ./configure --prefix=/apps/bread
# make -j4
# make install
# which gdb
```
- 由于8.3版本需要依赖gcc支持c++11，gdb必须等gcc安装完之后再安装

14. CMake安装
```shell
# wget https://github.com/Kitware/CMake/releases/download/v3# .14.5/cmake-3.14.5.tar.gz
# tar xvf cmake-3.14.5.tar.gz
# cd cmake-3.14.5
# ./configure --prefix=/apps/bread
# make -j4
# make install
# which cmake
```

15. 安装boost库
```shell
# yum -y install boost-devel
```

16. 安装yaml-cpp
```shell
方式一：下载的是最新版本0.7.0
注：该方式在自己测试程序中用cmake编译失败(因为cmake用到的是系统自带的默认路径的g++ 4.8.5)，要用自己上面安装的g++9.1.0。
# cd /soft
# git clone https://github.com/jbeder/yaml-cpp.git
# cd yaml-cpp
# mkdir build
# cd build
# cmake .. -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/apps/bread
# make -j4 & make install

方式二：下载的是无需依赖于boost的版本0.6.0
通过 https://codeload.github.com/jbeder/yaml-cpp/tar.gz/refs/tags/yaml-cpp-0.6.0 下载得到 yaml-cpp-yaml-cpp-0.6.0.tar.gz，
将 yaml-cpp-yaml-cpp-0.6.0.tar.gz 拷贝进虚拟机 /home/bread
注：该方式在自己测试程序中用cmake编译成功(因为cmake用到的是系统自带的g++ 4.8.5)，用自己上面安装的g++9.1.0编译失败。
# cd /home/bread
# mv yaml-cpp-yaml-cpp-0.6.0.tar.gz /soft
# cd /soft
# tar xvf yaml-cpp-yaml-cpp-0.6.0.tar.gz
# cd yaml-cpp-yaml-cpp-0.6.0
# mkdir build
# cd build
# source /etc/profile
# cmake .. -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/apps/bread
# make -j4 & make install
```

17. Ragel安装
```shell
# wget http://www.colm.net/files/ragel/ragel-6.10.tar.gz
# tar xvf ragel-6.10.tar.gz
# cd ragel-6.10
# ./configure --prefix=/apps/bread
# make -j4 & make install
# which ragel
```
- 有限状态机，编写出来的协议解析性能不弱于汇编。

18. 其他安装
- apache2-utils：Apache压力(并发)测试工具ab
    ```shell
    # yum -y install httpd-tools
    # ab -n 100 -c 10 http://www.baidu.com/               // 注意网址最后要带斜杠
    ```
- graphviz：一种dot工具可以用来渲染出效果更好的图表
    ```shell
    yum install graphviz
    ```
- doxygen：编写软件参考文档的工具
    ```shell
    yum -y install doxygen doxygen-latex doxygen-doxywizard
    ```
- psmisc：
    ```shell
    yum install psmisc
    ```
    - Psmisc软件包包含三个帮助管理/proc目录的程序。
        - fuser 显示使用指定文件或者文件系统的进程的PID。
        - killall 杀死某个名字的进程，它向运行指定命令的所有进程发出信号。
        - pstree 树型显示当前运行的进程。
- openssl
    ```shell
    yum install openssl-devel
    ```
- netstat
    ```shell
    yum install net-tools
    ```
- protobuf
    ```shell
    // 用wget也行，但是地址太长了，因此在 https://github.com/protocolbuffers/protobuf/releases 里面找到 protobuf-all-21.1.tar.gz，然后拷贝到虚拟机。
    // 先拷贝到 /home/bread 目录，然后再sudo mv 到 soft 目录。
    # tar -zxvf protobuf-all-21.1.tar.gz
    # cd protobuf-all-21.1
    # ./configure --prefix=/apps/bread          // 使用configure来指定安装路径
    # make -j4
    # make install
    # which protoc
    
    # vi /etc/profile 加上下面的内容（上面设置过环境变量了，这步暂不需要）
        #protobuf config
        #(动态库搜索路径) 程序加载运行期间查找动态链接库时指定除了系统默认路径之外的其他路径
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/apps/bread/protobuf/lib/
        #(静态库搜索路径) 程序编译期间查找动态链接库时指定查找共享库的路径
        export LIBRARY_PATH=$LIBRARY_PATH:/apps/bread/protobuf/lib/
        #执行程序搜索路径
        export PATH=$PATH:/apps/bread/protobuf/bin/
        #c程序头文件搜索路径
        export C_INCLUDE_PATH=$C_INCLUDE_PATH:/apps/bread/protobuf/include/
        #c++程序头文件搜索路径
        export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/apps/bread/protobuf/include/
        #pkg-config 路径
        export PKG_CONFIG_PATH=/apps/bread/protobuf/lib/pkgconfig/
    
    # source /etc/profile
    # protoc --version          // 居然报错，说 protoc: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.21' not found (required by protoc)，重启就好了。
    libprotoc 3.21.1
    ```

---

19. git 配置
```shell
1. 设置Git的user name和email
$ git config --global user.name "你GitHub用户名"
$ git config --global user.email "你GitHub的邮箱"

2. 生成密钥
$ ssh-keygen -t rsa -C "你GitHub的邮箱"      // 一路按 Enter 键即可

3. 检查是否已经有SSH Key
$ cat ~/.ssh/id_rsa.pub

4. 在 GitHub 上，账号处选择 Settings -> 左侧选择 SSH and GPG keys -> New SSH Key。

5. 在 GitHub 上，新建一个仓库，并选择初始化(因为实际项目中也是远端已有一个非空项目)。

6. clone 仓库到本地。 
$ git clone git@github.com:L-ge/awesome-sylar.git

7. 修改-上传代码到远端
$ cd awesome-sylar
$ vim README.md
$ git add README.md
$ git commit -m "项目启动"
$ git status
    # 位于分支 main
    # 您的分支领先 'origin/main' 共 1 个提交。
    #   （使用 "git push" 来发布您的本地提交）
    #
    无文件要提交，干净的工作区
$ git push -u origin main
    # 位于分支 main
    无文件要提交，干净的工作区

8. git add之后发现add错了一个文件tags，则
$ git reset HEAD tags

9. 本地update远端的最新代码
$ git pull origin main

10. 文件/文件夹重命名
$ git mv oldname newname

11. 比较更改内容
$ git diff a.txt		    // 比较工作区与暂存区(已add但未commit文件)的差异
$ git diff HEAD a.txt		// 比较工作区与本地仓库的差异
$ git diff --cached a.txt	// 比较暂存区与本地仓库的差异
```

20. doxygen 使用
```shell
$ cd /home/bread/workspace/awesome-sylar
$ doxygen -g -s         // 生成 Doxyfile 文件，-s为simple，-g为generate
注意要修改 vim Doxyfile 其中的属性 RECURSIVE = YES
$ doxygen               // 生成两个文件夹 html 和 latex
```

21. 代码补全设置
```shell
1. 假设项目代码在 /home/bread/workspace/awesome-sylar，则在该目录下执行
$ vctags
会在该目录下生成 tags 文件

2. 然后在 vim ~/.vimrc，在文件适当的位置加上
set tags+=~/workspace/awesome-sylar

3. vim insert模式下，Ctrl n就能全能补全。
```

22. 配置DoxygenToolkit.vim
```shell
$ git clone https://github.com/vim-scripts/DoxygenToolkit.vim
$ cd DoxygenToolkit.vim
$ cd plugin
$ cp DoxygenToolkit.vim /home/bread/.vim/plugin

// 然后在 ~/.vimrc 配置文件加上下面的内容：
""DoxygenToolkit
let g:DoxygenToolkit_briefTag_pre = "@brief\t"
let g:DoxygenToolkit_paramTag_pre = "@param\t"
let g:DoxygenToolkit_returnTag = "@return\t"
let g:DoxygenToolkit_authorName="L-ge"
let g:DoxygenToolkit_fileTag="@filename\t"
let g:DoxygenToolkit_dateTag = "@modify\t"
let g:DoxygenToolkit_classTag = "@class\t"
let g:DoxygenToolkit_authorTag = "@author\t"


完成后，可以在 vim normal 模式下输入 
:Dox			// 函数参数
:DoxAuthor		// 作者信息
:DoxLic			// 许可声明
```

23. 安装 man 手册
```shell
$ sudo yum install man-pages
```

24. 安装 Zlib
```shell
$ cd /soft
$ wget http://www.zlib.net/fossils/zlib-1.2.11.tar.gz
$ tar xvf zlib-1.2.11.tar.gz
$ ./configure --prefix=/apps/bread
$ make & make install
```

25. 安装 redis
```shell
// 不知道为啥在soft目录安装，即使是安装了tcl，make test也是报错说缺失。
// 怀疑是权限问题，因此在/home/bread/目录下安装redis成功。

$ cd /home/bread
$ mkdir soft
$ cd /soft
$ wget https://download.redis.io/releases/redis-5.0.14.tar.gz
$ tar xvf redis-5.0.14.tar.gz
$ cd redis-5.0.14
$ make			// 如果make失败报错缺失jemalloc，可以先make distclean

$ make test		
// 报错：You need tcl 8.5 or newer in order to run the Redis test

// 报错：*** [err]: Active defrag big keys in tests/unit/memefficiency.tcl Expected condition ‘$max_latency <= 120’ to be true (137 <= 120)
// 解决：vim tests/unit/memefficiency.tcl	改成 max_latency <= 150

$ cd ..
$ wget http://downloads.sourceforge.net/tcl/tcl8.6.1-src.tar.gz 
$ tar xvf tcl8.6.1-src.tar.gz 
$ cd tcl8.6.1/unix
$ ./configure --prefix=/apps/bread					
$ make & make install

$ cd /home/bread/soft/redis-5.0.14
$ make test
$ make install PREFIX=/apps/bread 		// 把redis安装到系统中(PREFIX为安装路径)

// 最后能在 /apps/bread/bin 目录下找到 redis_server 等可执行文件。
// 但仍然有问题，因为 /apps/bread/bin 的文件都是root权限的，普通用户使用redis-server时，将没有权限持久化(生成rdb文件需要写磁盘权限)。
// 因此，再将 make install PREFIX=/home/bread/soft 目录下，方便需要的时候cp过去。
```

26. 安装 hiredis
```shell
# cd /home/bread/soft
# git clone https://github.com/redis/hiredis.git		// 如果 Failed connect to github.com:443;  多试几次就行
# cd hiredis
# mkdir build
# cd build
# cmake .. -DCMAKE_INSTALL_PREFIX=/apps/bread
# make
# make install
```

27. 安装 json-cpp
```shell
// 用这个版本会有问题：https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.5.tar.gz
// 报错 ../lib/libsylar.so：对‘Json::Value::isMember(std::string const&) const’未定义的引用
// 原因未知
# cd /home/bread/soft
# wget https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/00.11.0.tar.gz
# tar xvf 00.11.0.tar.gz
# cd jsoncpp-00.11.0
# mkdir build
# cd build
# source /etc/profile
# cmake .. -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/apps/bread
# make -j4 & make install
```
