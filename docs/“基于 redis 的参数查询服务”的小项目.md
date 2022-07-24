# 需求
- 设计上考虑一亿条记录。
- Http Get请求，json格式返回。
- 要求内存越小越好、要求在网络正常的情况下查询耗时不超过20ms。
- 参数表格式如下：
	```
	cardId	type	status
	44016758092746254872	7	2
	45018537070476554829	6	2
	...
	```
	- 其中，cardId 为字符串；type 为不超过 10 的纯数字；status 为 1 或 2。

# 设计思想
- 一亿条记录，对耗时有要求，因此用 redis 实现。
- 对内存使用有要求，因此利用 redis 中 hash 数据结构，控制数据使其内部编码为 ziplist。
- 指令组成：hset key field value
	- key 的计算方法为：
		```
		方式一：
		std::hash<std::string> str_hash;
		key = std::to_string(str_hash(sKey) % 300000);
		```
		- 哈希类型的内部编码有两种：
			- ziplist(压缩列表)：当哈希类型元素个数小于hash-max-ziplist-entries配置(默认512个)、同时所有值都小于hash-max-ziplist-value配置(默认64字节)时，Redis会使用ziplist作为哈希的内部实现，ziplist使用更加紧凑的结构实现多个元素的连续存储，所以在节省内存方面比hashtable更加优秀。
			- hashtable(哈希表)：当哈希类型无法满足ziplist的条件时，Redis会使用hashtable作为哈希的内部实现，因为此时ziplist的读写效率会下降，而hashtable的读写时间复杂度为O(1)。
		- 因此，1亿条记录，每个桶大概装500对键值对，大概需要200000个桶，为避免hash冲突，取300000个桶，因此取余300000。
	- field 的计算方法为：
		```
		unsigned int BKDRHash(const char *str) {
			unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
			unsigned int hash = 0;
			while (*str) {
				hash = hash * seed + (*str++);
			}
			return (hash & 0x7FFFFFFF);
		}
		unsigned int nFieldKey = BKDRHash(sKey.c_str());
		```
		- 落到同一个桶的记录都是第一次求hash值取余后碰撞上的，因此这一步求field，使用另一个hash函数求值。
	- value 的计算方法为：
		```
		void encodeValue(const std::string& sStatus, const std::string& sType, std::string& sResult)
		{
			unsigned long nStatus = std::stoul(sStatus);
			unsigned long nType = std::stoul(sType);
			unsigned long nHashValue = 0x00;
			nHashValue = (nStatus << 6) & 0xC0;
			nHashValue |= (nType & 0x3F);
			sResult = std::to_string(nHashValue);
		}
		
		void decodeValue(std::string& sStatus, std::string& sType, const std::string& sResult)
		{
			unsigned long nHashValue = std::stoul(sResult);
			unsigned long nStatus = (nHashValue & 0xC0) >> 6;
			unsigned long nType = nHashValue & 0x3F;
			sStatus = std::to_string(nStatus);
			sType = std::to_string(nType);
		}
		```
		- type和status合并成value，且仅取一个字节（最低位字节）。
		- value的最低位字节的低6位用来表示type，最低位字节的高2位用来表示status，冗余空间用做日后扩展。
- 参数下发后放到指定目录，独立线程定时检查是否有参数更新，如果有参数更新，则预处理成redis协议的参数格式，例如：
	```
	// 预处理前
	cardId	type	status
	44016758092746254872	7	2
	
	// 预处理后
	*4
	$4
	hset
	$6
	252724
	$9
	143964783
	$3
	135
	```
	- 这里有个小坑，预处理的时候：windows下用\n，Linux下用\r\n，否则有如下报错：
		`ERR Protocol error: invalid multibulk length`
- 数据量较大，因此预处理后redis协议格式的参数文件通过如下命令进行导入到redis-server中：
	`cat xxx.dat | redis-cli --pipe`
- 请求和回复样式：
	```
	url:http://192.168.60.138:8020/paramquery?cardid=2030230000000002&cardnet=4401&cmdtype=blacklistquery&serialno=1005&tbtype=0&version=201708031
	ret:{"cmdtype":"blacklistqueryresult","errorcode":"0","recordcnt":"0","serialno":"1005","version":"201708031"}


	url:http://192.168.60.138:8020/paramquery?cardid=5186100181001473&cardnet=4401&cmdtype=blacklistquery&serialno=1011&tbtype=0&version=201708031
	ret:{"cmdtype":"blacklistqueryresult","darkstatus0":"1","darktype0":"7","darkver0":"1970-01-01 08:00:00","errorcode":"0","recordcnt":"1","serialno":"1011","status":"1","type":"7","version":"201708031"}
	```

# 测试情况
- 虚拟机概况：CentOs7、4核处理器(2×2)、4GB内存、固态硬盘。
- 数据量1亿，预处理耗时：251622ms ≈ 4.19分钟。
- 预处理前参数文件大小2.32GB，预处理后参数文件大小4.7GB。
- 每个桶大概300+个键值对，ziplist编码。
- 预处理后参数导入redis耗时：387242ms ≈ 6.45分钟。
- 重新启动后参数导入redis耗时：408096ms = 6.8分钟。
- 查询耗时：2-6ms之间。
- 对预处理后的hash结果与预处理前参数文件进行数据检验，发现误差情况如下：
	- 第一次和第二次hash都一样的记录有 16 条。
	- *可以选用更好的hash函数使其碰撞率降低。(待完善、待测试)*
- 具体运行情况：
	```
	# 加载参数前 Memory
	used_memory:595720				// Redis分配器分配的内存总量，也就是内部存储的所有数据内存占用量
	used_memory_human:581.76K		// 以可读的格式返回used_memory
	used_memory_rss:10936320
	used_memory_rss_human:10.43M	// 从操作系统的角度，Redis进程占用的物理内存总量
	used_memory_peak:595720
	used_memory_peak_human:581.76K	// 内存使用的最大值，表示used_memory的峰值
	used_memory_peak_perc:100.01%
	used_memory_overhead:579522
	used_memory_startup:512904
	used_memory_dataset:16198
	used_memory_dataset_perc:19.56%
	allocator_allocated:1219304
	allocator_active:1552384
	allocator_resident:8634368
	total_system_memory:3953926144
	total_system_memory_human:3.68G
	used_memory_lua:37888
	used_memory_lua_human:37.00K
	used_memory_scripts:0
	used_memory_scripts_human:0B
	number_of_cached_scripts:0
	maxmemory:0
	maxmemory_human:0B
	maxmemory_policy:noeviction
	allocator_frag_ratio:1.27
	allocator_frag_bytes:333080
	allocator_rss_ratio:5.56
	allocator_rss_bytes:7081984
	rss_overhead_ratio:1.27
	rss_overhead_bytes:2301952
	mem_fragmentation_ratio:19.72		// used_memory_rss/used_memory比值，表示内存碎片率
	mem_fragmentation_bytes:10381624
	mem_not_counted_for_evict:0
	mem_replication_backlog:0
	mem_clients_slaves:0
	mem_clients_normal:66618
	mem_aof_buffer:0
	mem_allocator:jemalloc-5.1.0
	active_defrag_running:0
	lazyfree_pending_objects:0

	
	# 更新参数后 Memory
	used_memory:1053488904
	used_memory_human:1004.69M
	used_memory_rss:1356754944
	used_memory_rss_human:1.26G
	used_memory_peak:1053488904
	used_memory_peak_human:1004.69M
	used_memory_peak_perc:100.00%
	used_memory_overhead:16773826
	used_memory_startup:512904
	used_memory_dataset:1036715078
	used_memory_dataset_perc:98.46%
	allocator_allocated:1053561192
	allocator_active:1305292800
	allocator_resident:1357246464
	total_system_memory:3953926144
	total_system_memory_human:3.68G
	used_memory_lua:37888
	used_memory_lua_human:37.00K
	used_memory_scripts:0
	used_memory_scripts_human:0B
	number_of_cached_scripts:0
	maxmemory:0
	maxmemory_human:0B
	maxmemory_policy:noeviction
	allocator_frag_ratio:1.24
	allocator_frag_bytes:251731608
	allocator_rss_ratio:1.04
	allocator_rss_bytes:51953664
	rss_overhead_ratio:1.00
	rss_overhead_bytes:-491520
	mem_fragmentation_ratio:1.29
	mem_fragmentation_bytes:303307056
	mem_not_counted_for_evict:0
	mem_replication_backlog:0
	mem_clients_slaves:0
	mem_clients_normal:66618
	mem_aof_buffer:0
	mem_allocator:jemalloc-5.1.0
	active_defrag_running:0
	lazyfree_pending_objects:0

	
	# top指令查看结果
	 PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
	3168 bread     20   0 1869256   1.3g   1284 S   0.3 34.3   6:27.41 redis-server
	```

# 其他事项

- 避免持久化阻塞redis，redis配置为不持久化(redis.conf)：
	```
	save 900 1	--->	# save 900 1
	save 300 10	--->	# save 300 10
	save 60 10000	--->	# save 60 10000
	```
- 为能远程到redis-server，修改如下配置(redis.conf)：
	```
	bind 127.0.0.1	--->	# bind 127.0.0.1
	protected-mode yes	--->	protected-mode no	
	```
