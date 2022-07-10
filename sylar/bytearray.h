/**
 * @filename    bytearray.h
 * @brief   ByteArray 模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-10
 */
#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace sylar
{

/**
 * @brief   二进制数组，提供基础类型的序列化和反序列化功能
 */
class ByteArray
{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief  存储节点 
     */
    struct Node
    {
        /**
         * @brief  构造指定大小的内存块 
         *
         * @param   s   内存块字节数
         */
        Node(size_t s);
        Node();
        ~Node();

        /// 内存块地址指针
        char* ptr;
        /// 下一个内存块地址
        Node* next;
        /// 内存块大小
        size_t size;
    };

    /**
     * @brief   按指定大小的内存块构造
     *
     * @param   base_size   内存块大小
     */
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    /**
     * @brief  写入固定长度类型的整型数据 
     */
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);


    /**
     * @brief  写入可压缩(Zigzag算法)的字符串数据 
     */
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);
    
    void writeFloat(float value);
    void writeDouble(double value);

    /**
     * @brief  写入前面带长度(长度所占字节数固定)的字符串数据(长度+实际数据) 
     */
    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);
    
    /**
     * @brief  写入前面带长度的数据(实际长度+实际数据) 
     *         长度所占字节数为可压缩的uint64的实际大小
     */
    void writeStringVint(const std::string& value);

    /**
     * @brief   写入不带长度的字符串数据
     */
    void writeStringWithoutLength(const std::string& value);

    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();
    
    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    void clear();
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);

    /**
     * @brief  读取size长度的数据 
     *
     * @param   position    读取开始的位置
     */
    void read(void* buf, size_t size, size_t position) const;
    
    /**
     * @brief  返回ByteArray的当前位置 
     */
    size_t getPosition() const { return m_position; }
    void setPosition(size_t v);
    
    /**
     * @brief  把ByteArray的数据写入到文件中 
     */
    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);
    
    /**
     * @brief  返回内存块的大小 
     */
    size_t getBaseSize() const { return m_baseSize; }
    
    /**
     * @brief   返回可读取数据的大小
     */
    size_t getReadSize() const { return m_size - m_position; }

    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    std::string toString() const;
    std::string toHexString() const;

    /**
     * @brief   获取可读取的缓存，保存到iovec数组中 
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
  
    /**
     * @brief   获取可读取的缓存，保存到iovec数组中，从position位置开始
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    
    /**
     * @brief   获取可写入的缓存，保存到iovec数组中
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    /**
     * @brief   返回数据的长度(当前总数据量)
     */
    size_t getSize() const { return m_size; }

private:
    /**
     * @brief   扩容ByteArray，使其可以容纳size个数据 
     */
    void addCapacity(size_t size);
    
    /**
     * @brief   获取当前的可写入容量
     */
    size_t getCapacity() const { return m_capacity - m_position; }

private:
    /// 内存块的大小 
    size_t m_baseSize;
    /// 当前操作位置
    size_t m_position;
    /// 当前的总容量
    size_t m_capacity;
    /// 当前总数据量
    size_t m_size;
    /// 字节序,默认大端
    int8_t m_endian;
    /// 第一个内存块的指针
    Node* m_root;
    /// 当前操作内存块的指针
    Node* m_cur;
};

}
#endif
