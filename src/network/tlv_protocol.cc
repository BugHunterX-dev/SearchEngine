#include "tlv_protocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>

using std::cout;
using std::endl;
using std::string;
using std::vector;

// ============ TLVMessage 实现 ============
TLVMessage::TLVMessage(MessageType msgType, const string& jsonData)
    : header(msgType, jsonData.length())
    , data(jsonData.begin(), jsonData.end()) {}

string TLVMessage::getJsonData() const {
    return string(reinterpret_cast<const char*>(data.data()), data.size());
}

// ============ TLVCodec 实现 ============
vector<uint8_t> TLVCodec::encode(const TLVMessage& message) {
    vector<uint8_t> buffer;

    // 将消息头转换为网络字节序并写入缓冲区
    TLVHeader networkHeader;
    networkHeader.type = htons(message.header.type);
    networkHeader.length = htonl(message.header.length);

    // 写入头部
    buffer.resize(sizeof(TLVHeader));
    std::memcpy(buffer.data(), &networkHeader, sizeof(TLVHeader));

    // 写入数据部分
    buffer.insert(buffer.end(), message.data.begin(), message.data.end());

    return buffer;
}

vector<TLVMessage> TLVCodec::decode(const vector<uint8_t>& buffer,
                                    size_t& parsedBytes) {
    vector<TLVMessage> messages;
    parsedBytes = 0;

    // 每次循环前判断：是否还有至少一个完整的 TLVHeader 可供解析
    // 避免 buffer 尾部是粘包残留，导致非法访问或数据不完整
    while (parsedBytes + sizeof(TLVHeader) <= buffer.size()) {
        // 解析消息头
        TLVHeader networkHeader;
        std::memcpy(&networkHeader, buffer.data() + parsedBytes,
                    sizeof(TLVHeader));

        // 转换为主机字节序
        TLVHeader hostHeader;
        hostHeader.type = ntohs(networkHeader.type);
        hostHeader.length = ntohl(networkHeader.length);

        // 检查消息类型是否有效
        if (!isValidMessageType(hostHeader.type)) {
            cout << "无效的消息类型: " << hostHeader.type << endl;
            ++parsedBytes; // 跳过一个字节继续解析
            continue;
        }

        // 检查是否有完整的消息数据
        size_t totalMessageSize = sizeof(TLVHeader) + hostHeader.length;
        if (parsedBytes + totalMessageSize > buffer.size()) {
            // 数据不完整，等待更多数据
            break;
        }

        // 创建TLV消息
        TLVMessage message;
        message.header = hostHeader;

        // 复制数据部分
        if (hostHeader.length > 0) {
            message.data.resize(hostHeader.length);
            std::memcpy(message.data.data(),
                        buffer.data() + parsedBytes + sizeof(TLVHeader),
                        hostHeader.length);
        }

        messages.push_back(std::move(message));
        parsedBytes += totalMessageSize;
    }

    return messages;
}

bool TLVCodec::hasCompleteMessage(const vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TLVHeader)) {
        return false;
    }

    // 解析消息头获取数据长度
    TLVHeader networkHeader;
    std::memcpy(&networkHeader, buffer.data(), sizeof(TLVHeader));
    uint32_t dataLength = ntohl(networkHeader.length);

    // 检查是否有完整消息
    return buffer.size() >= sizeof(TLVHeader) + dataLength;
}

size_t TLVCodec::getRequiredBytes(const vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TLVHeader)) {
        return sizeof(TLVHeader) - buffer.size();
    }

    // 解析消息头获取数据长度
    TLVHeader networkHeader;
    std::memcpy(&networkHeader, buffer.data(), sizeof(TLVHeader));
    uint32_t dataLength = ntohl(networkHeader.length);

    size_t totalRequired = sizeof(TLVHeader) + dataLength;
    if (buffer.size() >= totalRequired) {
        return 0; // 已经有完整消息
    }

    return totalRequired - buffer.size();
}

bool TLVCodec::isValidMessageType(uint16_t type) {
    switch (static_cast<MessageType>(type)) {
    case MessageType::KEYWORD_RECOMMEND_REQUEST:
    case MessageType::SEARCH_REQUEST:
    case MessageType::KEYWORD_RECOMMEND_RESPONSE:
    case MessageType::SEARCH_RESPONSE:
    case MessageType::ERROR_RESPONSE:
        return true;
    default:
        return false;
    }
}

// ============ TLVMessageBuilder 实现 ============
TLVMessage TLVMessageBuilder::buildKeywordRecommendRequest(const string& query,
                                                           int k) {
    nlohmann::json requestJson = {
        {"query", query},
        {"k", k},
        {"timestamp", static_cast<int>(std::time(nullptr))}};

    return TLVMessage(MessageType::KEYWORD_RECOMMEND_REQUEST,
                      requestJson.dump());
}

TLVMessage TLVMessageBuilder::buildSearchRequest(const string& query,
                                                 int topN) {
    nlohmann::json requestJson = {
        {"query", query},
        {"topN", topN},
        {"timestamp", static_cast<int>(std::time(nullptr))}};

    return TLVMessage(MessageType::SEARCH_REQUEST, requestJson.dump());
}

TLVMessage
TLVMessageBuilder::buildKeywordRecommendResponse(const string& jsonResponse) {
    return TLVMessage(MessageType::KEYWORD_RECOMMEND_RESPONSE, jsonResponse);
}

TLVMessage TLVMessageBuilder::buildSearchResponse(const string& jsonResponse) {
    return TLVMessage(MessageType::SEARCH_RESPONSE, jsonResponse);
}

TLVMessage TLVMessageBuilder::buildErrorResponse(const string& errorMessage,
                                                 int errorCode) {
    nlohmann::json errorJson = {
        {"error", errorMessage},
        {"code", errorCode},
        {"timestamp", static_cast<int>(std::time(nullptr))}};

    return TLVMessage(MessageType::ERROR_RESPONSE, errorJson.dump());
}