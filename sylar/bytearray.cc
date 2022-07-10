#include "bytearray.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <math.h>
#include "log.h"
#include "endian.h"

namespace sylar
{
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ByteArray::Node::Node(size_t s)
    : ptr(new char[s])
    , next(nullptr)
    , size(s)
{

}

ByteArray::Node::Node()
    : ptr(nullptr)
    , next(nullptr)
    , size(0)
{

}

ByteArray::Node::~Node()
{
    if(ptr)
    {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size)
    , m_position(0)
    , m_capacity(base_size)
    , m_size(0)
    , m_endian(SYLAR_BIG_ENDIAN)
    , m_root(new Node(base_size))
    , m_cur(m_root)
{

}

ByteArray::~ByteArray()
{
    Node* tmp = m_root;
    while(tmp)
    {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;   // 调用Node的析构函数，里面再delete掉一个节点所占的char数组
    }
}

void ByteArray::writeFint8(int8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value)
{
    if(m_endian != SYLAR_BYTE_ORDER)
    {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value)
{
    if(m_endian != SYLAR_BYTE_ORDER)
    {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value)
{
    if(m_endian != SYLAR_BYTE_ORDER)
    {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value)
{
    if(m_endian != SYLAR_BYTE_ORDER)
    {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value)
{
    if(m_endian != SYLAR_BYTE_ORDER)
    {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value)
{
    if(m_endian != SYLAR_BYTE_ORDER)
    {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v)
{
    if(v < 0)
    {
        return ((uint32_t)(-v)) * 2 - 1;
    }
    else
    {
        return v * 2;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v)
{
    if(v < 0)
    {
        return ((uint64_t)(-v)) * 2 - 1;
    }
    else
    {
        return v * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v)
{
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v)
{
    return (v >> 1) ^ -(v & 1);
}

void ByteArray::writeInt32(int32_t value)
{
    writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32(uint32_t value)
{
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80)
    {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64(int64_t value)
{
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value)
{
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80)
    {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat(float value)
{
    uint32_t v;
    memcpy(&v, &value, sizeof(value));  // 当作固定长度的uint32_t写入
    writeFuint32(v);
}

void ByteArray::writeDouble(double value)
{
    uint64_t v;
    memcpy(&v, &value, sizeof(value));  // 当作固定长度的uint64_t写入
    writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string& value)
{
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string& value)
{
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string& value)
{
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string& value)
{
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string& value)
{
    write(value.c_str(), value.size());
}

int8_t ByteArray::readFint8()
{
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8()
{
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == SYLAR_BYTE_ORDER) \
    { \
        return v; \
    } \
    else \
    { \
        return byteswap(v); \
    }

int16_t ByteArray::readFint16()
{
    XX(int16_t);
}

uint16_t ByteArray::readFuint16()
{
    XX(uint16_t);
}

int32_t ByteArray::readFint32()
{
    XX(int32_t);
}

uint32_t ByteArray::readFuint32()
{
    XX(uint32_t);
}

int64_t ByteArray::readFint64()
{
    XX(int64_t);
}

uint64_t ByteArray::readFuint64()
{
    XX(uint64_t);
}

#undef XX

int32_t ByteArray::readInt32()
{
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32()
{
    uint32_t result = 0;
    for(int i=0; i<32; i+=7)
    {
        uint8_t b = readFuint8();
        if(b < 0x80)
        {
            result |= ((uint32_t)b) << i;
            break;
        }
        else
        {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64()
{
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64()
{
    uint64_t result = 0;
    for(int i=0; i<64; i+=7)
    {
        uint8_t b = readFuint8();
        if(b < 0x80)
        {
            result |= ((uint64_t)b) << i;
            break;
        }
        else
        {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

float ByteArray::readFloat()
{
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double ByteArray::readDouble()
{
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16()
{
    uint16_t len = readFuint16();    // 先把长度读出来，再读实际数据(字符串)
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32()
{
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64()
{
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint()
{
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear()
{
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;   // 保留根节点
    while(tmp)
    {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = NULL;
}

void ByteArray::write(const void* buf, size_t size)
{
    if(size == 0)
    {
        return;
    }
    addCapacity(size);      // 先根据需要扩充容量

    size_t npos = m_position % m_baseSize;  // 当前内存块所用的字节数
    size_t ncap = m_cur->size - npos;       // 当前内存块所剩下的字节数
    size_t bpos = 0;            // 已经分给使用者的内存字节数
    while(size > 0)
    {
        if(ncap >= size)  // 当前内存块所剩下的内存够用
        {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            if(m_cur->size == (npos + size)) // 当前内存块已用完，更新当前可用内存块位置
            {
                m_cur = m_cur->next;
            }
            m_position += size;     // 当前位置变更
            bpos += size;           // 分配给buf的位置更新
            size = 0;               // 所要的内存字节数变0，也就是满足需求了
        }
        else
        {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap); // 先分完当前的内存块
            m_position += ncap;     // 当前位置变更，加上已经分的字节数
            bpos += ncap;           // 已经分给使用者ncap个字节
            size -= ncap;           // 需求量减少ncap个字节
            m_cur = m_cur->next;    // 当前内存块已经用完，更新当前可用内存块
            ncap = m_cur->size;     // 下一个内存块的所剩下字节数，也就是下一块内存块的大小
            npos = 0;               // 下一个内存块是刚刚才next到的，所用字节数肯定是0
        }
    }

    // 当前位置比当前使用内存量大，则更新当前内存使用量
    if(m_position > m_size)
    {
        m_size = m_position;
    }
}

void ByteArray::read(void* buf, size_t size)
{
    // 需要读取的比可读取的还要大，则认为异常
    if(size > getReadSize())
    {
        throw std::out_of_range("not enough len");
    }

    // 下面读取的代码和写入的代码的区别只是在memcpy的时候，
    // 读取时 memcpy源地址和目的地址 与 写入时调转
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    while(size > 0)
    {
        if(ncap >= size)
        {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size))
            {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        }
        else
        {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t position) const
{
    // 总使用量减去要读取的位置，也就是能读到的字节数。
    // 需要读取的比能读到的要大，则认为异常。
    if(size > (m_size - position))
    {
        throw std::out_of_range("not enough len");
    }

    // 下面代码和不指定位置读取的代码的区别是：
    // 1、不使用m_position；2、没有副作用，不直接改m_cur
    size_t npos = position % m_baseSize;    
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;      // 无副作用，不影响原有数据结构
    while(size > 0)
    {
        if(ncap >= size)
        {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size))
            {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        }
        else
        {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

void ByteArray::setPosition(size_t v)
{
    // 要设置的当前位置比容量还大，则认为异常
    if(v > m_capacity)
    {
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    
    if(m_position > m_size)
    {
        m_size = m_position;
    }

    // 因为当前位置变了，那么指向当前内存块的指针也要变
    m_cur = m_root;
    while(v > m_cur->size)
    {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size)
    {
        m_cur = m_cur->next;
    }
}

bool ByteArray::writeToFile(const std::string& name) const
{
    std::ofstream ofs;
    // trunc覆盖写，binary以二进制模式打开
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs)
    {
        SYLAR_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

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

bool ByteArray::readFromFile(const std::string& name)
{
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs)
    {
        SYLAR_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr; });
    while(!ifs.eof())
    {
        ifs.read(buff.get(), m_baseSize);   // 每次读取一块
        write(buff.get(), ifs.gcount());    // gcount()返回最近无格式输入操作所提取的字符数
    }
    return true;
}

bool ByteArray::isLittleEndian() const
{
    return m_endian == SYLAR_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val)
{
    if(val)
    {
        m_endian = SYLAR_LITTLE_ENDIAN;
    }
    else
    {
        m_endian = SYLAR_BIG_ENDIAN;
    }
}

std::string ByteArray::toString() const
{
    std::string str;
    str.resize(getReadSize());
    if(str.empty())
    {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

std::string ByteArray::toHexString() const
{
    std::string str = toString();
    std::stringstream ss;
    for(size_t i=0; i<str.size(); ++i)
    {
        if(i > 0 && i % 32 == 0)
        {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }
    return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0)
    {
        return 0;
    }

    uint64_t size = len;
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0)
    {
        if(ncap >= len)
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const
{
    // 这里的判断好像没啥用，因为下面用的position是外面传进来的，
    // 与m_position是没有关系的，bug???
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0)
    {
        return 0;
    }

    uint64_t size = len;
    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0)
    {
        cur = cur->next;
        --count;
    }
    
    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0)
    {
        if(ncap >= len)
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len)
{
    if(len == 0)
    {
        return 0;
    }
    addCapacity(len);

    uint64_t size = len;
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0)
    {
        if(ncap >= len)
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

void ByteArray::addCapacity(size_t size)
{
    if(size == 0)
    {
        return;
    }

    size_t old_cap = getCapacity();
    if(old_cap >= size)     // 剩余容量还够，无需扩充
    {
        return;
    }

    size = size - old_cap;  // 得到需要扩充的容量大小
    // ceil(x) 返回大于或者等于x的最小整数
    size_t count = ceil(1.0 * size / m_baseSize);   // 得到需要扩充的块数
    
    // 遍历得到最后一个节点
    Node* tmp = m_root;
    while(tmp->next)
    {
        tmp = tmp->next;
    }

    Node* first = NULL;     // 记录扩充的第一块内存块的位置
    for(size_t i=0; i<count; ++i)
    {
        tmp->next = new Node(m_baseSize);
        if(first == NULL)
        {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;   // 容量加上一个内存块
    }

    // 如果剩余容量已经是0了，那么扩充的第一块内容块就是当前可用内存块所在的位置
    if(old_cap == 0)        
    {
        m_cur = first;
    }
}

}

