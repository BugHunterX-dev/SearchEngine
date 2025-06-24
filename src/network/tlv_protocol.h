#pragma once

#include <cstdint>
#include <string>
#include <vector>

// TLV消息类型定义
enum class MessageType : uint16_t {
    // 请求类型
    KEYWORD_RECOMMEND_REQUEST = 0x0001, // 关键字推荐请求
    SEARCH_REQUEST = 0x0002,            // 搜索请求

    // 响应类型
    KEYWORD_RECOMMEND_RESPONSE = 0x1001, // 关键字推荐响应
    SEARCH_RESPONSE = 0x1002,            // 搜索响应

    // 错误类型
    ERROR_RESPONSE = 0x9001 // 错误响应
};

// TLV消息头结构 (固定6字节)
struct TLVHeader {
    uint16_t type;   // 消息类型 (2字节)
    uint32_t length; // 数据长度 (4字节，不包括头部长度)

    TLVHeader(MessageType msgType = MessageType::ERROR_RESPONSE,
              uint32_t dataLen = 0)
        : type(static_cast<uint16_t>(msgType))
        , length(dataLen) {}
} __attribute__((packed));

// TLV消息完整结构
struct TLVMessage {
    TLVHeader header;          // 消息头
    std::vector<uint8_t> data; // 消息数据（JSON格式）

    TLVMessage() = default;
    TLVMessage(MessageType msgType, const std::string& jsonData);

    // 获取消息类型
    MessageType getType() const {
        return static_cast<MessageType>(header.type);
    }

    // 获取JSON数据
    std::string getJsonData() const;

    // 获取消息总长度（包括头部）
    size_t getTotalLength() const {
        return sizeof(TLVHeader) + header.length;
    }
};

// TLV协议编解码器
class TLVCodec {
public:
    // 编码：将TLV消息转换为字节流
    static std::vector<uint8_t> encode(const TLVMessage& message);

    // 解码：从字节流解析TLV消息
    // 返回解析的消息数量，parsedBytes返回已解析的字节数
    static std::vector<TLVMessage> decode(const std::vector<uint8_t>& buffer,
                                          size_t& parsedBytes);

    // 检查缓冲区是否包含完整的TLV消息
    static bool hasCompleteMessage(const std::vector<uint8_t>& buffer);

    // 获取下一个完整消息需要的字节数
    static size_t getRequiredBytes(const std::vector<uint8_t>& buffer);

private:
    // 检查消息类型是否有效
    static bool isValidMessageType(uint16_t type);
};

// 请求响应辅助函数
class TLVMessageBuilder {
public:
    // 构建关键字推荐请求
    static TLVMessage buildKeywordRecommendRequest(const std::string& query,
                                                   int k = 10);

    // 构建搜索请求
    static TLVMessage buildSearchRequest(const std::string& query,
                                         int topN = 5);

    // 构建关键字推荐响应
    static TLVMessage
    buildKeywordRecommendResponse(const std::string& jsonResponse);

    // 构建搜索响应
    static TLVMessage buildSearchResponse(const std::string& jsonResponse);

    // 构建错误响应
    static TLVMessage buildErrorResponse(const std::string& errorMessage,
                                         int errorCode = -1);
};