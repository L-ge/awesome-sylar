# 概述

- 二进制数组（序列化/反序列化）模块。
- 字节数组容器，提供基础类型的序列化与反序列化功能。
- ByteArray 的底层存储是固定大小的块，以链表形式组织。每次写入数据时，将数据写入到链表最后一个块中，如果最后一个块不足以容纳数据，则分配一个新的块并添加到链表结尾，再写入数据。ByteArray 会记录当前的操作位置，每次写入数据时，该操作位置按写入大小往后偏移，如果要读取数据，则必须调用 setPosition 重新设置当前的操作位置。
- ByteArray 支持基础类型的序列化与反序列化功能，并且支持将序列化的结果写入文件，以及从文件中读取内容进行反序列化。
- ByteArray 支持以下类型的序列化与反序列化：
	- 固定长度的有符号/无符号8位、16位、32位、64位整数。
    - 不固定长度的有符号/无符号32位、64位整数。
    - float、double 类型。
    - 字符串，包含字符串长度，长度范围支持16位、32位、64位。
    - 字符串，不包含长度。
	- 以上所有的类型都支持读写。
- ByteArray 还支持设置序列化时的大小端顺序。


# ByteArray

- 二进制数组，提供基础类型的序列化，反序列化功能。
- 内含一个 Node 类，作为 ByteArray 的存储节点。
- 相当于一个链表的数据结构。


# 其他说明

- ByteArray 在序列化不固定长度的有符号/无符号32位、64位整数时使用了 zigzag 算法。zigzag 算法参考： [小而巧的数字压缩算法：zigzag](https://blog.csdn.net/zgwangbo/article/details/51590186)
- ByteArray 在序列化字符串时使用 TLV 编码结构中的 Length 和 Value。TLV编码结构用于序列化和消息传递，指 Tag（类型），Length（长度），Value（值），参考：[TLV编码通信协议设计](https://www.wtango.com/tlv%E7%BC%96%E7%A0%81%E9%80%9A%E4%BF%A1%E5%8D%8F%E8%AE%AE%E8%AE%BE%E8%AE%A1/)


# 待完善

- 感觉这个模块有两个地方有bug。
	- ByteArray::getReadBuffers 方法：
	```C++
	uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const
	{
    // 这里的判断好像没啥用，因为下面用的position是外面传进来的，
    // 与m_position是没有关系的，bug???
    len = len > getReadSize() ? getReadSize() : len;
    ...
	}
	```
	- ByteArray::writeToFile 方法：
	```C++
	bool ByteArray::writeToFile(const std::string& name) const
	{
    ...
    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_cur;
    while(read_size > 0)
    {
        int diff = pos % m_baseSize;    // 当前块不可读(已经读完)的字节数
	
        // 1、要读的内容比一整块要大，则读完一整块剩下的那些
        // 2、要读的内容比一整块少，则读？？？
        // 假设一块10字节，可读9字节，不可读是3字节，则读9-3=6字节，bug???
        // 假设一块10字节，可读1字节，不可读是3字节，则读1-3=-5字节，bug???
        // 应该是：
        // int64_t len = 0;
        // if(read_size > (int64_t)m_baseSize - diff)
        //      len = (int64_t)m_baseSize - diff;
        // else
        //      len = read_size;
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }
	
    return true;
	}
	```
