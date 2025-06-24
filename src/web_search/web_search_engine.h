#pragma once

#include "../data_reader/data_reader.h"
#include "../keywords/common.h"
#include "../cache/lru_cache.h"
#include <cppjieba/Jieba.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// 搜索结果项结构
struct SearchResult {
    int docid;           // 文档ID
    std::string title;   // 网页标题
    std::string url;     // 网页链接
    std::string summary; // 网页摘要
    double score;        // 相关性得分

    SearchResult(int id = 0, const std::string& t = "",
                 const std::string& u = "", const std::string& s = "",
                 double sc = 0.0)
        : docid{id}
        , title{t}
        , url{u}
        , summary{s}
        , score{sc} {}

    // 转换为JSON格式，带错误处理
    nlohmann::json toJson() const {
        try {
            // 尝试创建JSON对象
            return nlohmann::json{{"docid", docid},
                                  {"title", title},
                                  {"url", url},
                                  {"summary", summary},
                                  {"score", score}};
        } catch (const std::exception& e) {
            // 如果JSON序列化失败，返回一个安全的替代JSON
            return nlohmann::json{{"docid", docid},
                                  {"title", "标题无法显示"},
                                  {"url", url},
                                  {"summary", "内容无法显示"},
                                  {"score", score}};
        }
    }
};

// 搜索结果比较器（按得分降序排列）
struct SearchResultComparator {
    bool operator()(const SearchResult& a, const SearchResult& b) const {
        // 优先比较得分（得分高的优先）
        if (a.score != b.score) {
            return a.score > b.score;
        }
        // 得分相同时，按docid升序（保证结果稳定）
        return a.docid < b.docid;
    }
};

// 搜索缓存键结构
struct SearchCacheKey {
    std::string query;
    int topN;
    
    bool operator==(const SearchCacheKey& other) const {
        return query == other.query && topN == other.topN;
    }
    
    bool operator<(const SearchCacheKey& other) const {
        if (query != other.query) return query < other.query;
        return topN < other.topN;
    }
};

// 为搜索缓存键提供hash函数
namespace std {
    template<>
    struct hash<SearchCacheKey> {
        std::size_t operator()(const SearchCacheKey& key) const {
            return std::hash<std::string>()(key.query) ^ 
                   (std::hash<int>()(key.topN) << 1);
        }
    };
}

// 网页搜索引擎类
class WebSearchEngine {
public:
    // 构造函数
    explicit WebSearchEngine(DataReaderManager* dataManager);
    ~WebSearchEngine() = default;

    // 搜索接口
    std::vector<SearchResult> search(const std::string& query, int topN = 5);

    // 搜索结果转JSON
    nlohmann::json searchToJson(const std::string& query, int topN = 5);

    // 设置摘要长度
    void setSummaryLength(int length) {
        maxSummaryLength_ = length;
    }
    
    // 缓存管理
    void setCacheCapacity(size_t searchCacheCapacity = 200) {
        searchCache_.setCapacity(searchCacheCapacity);
    }
    
    // 获取缓存统计信息
    LRUCache<SearchCacheKey, std::vector<SearchResult>>::CacheStats getCacheStats() const {
        return searchCache_.getStats();
    }
    
    // 清空缓存
    void clearCache() {
        searchCache_.clear();
    }

private:
    // 核心搜索算法
    std::vector<SearchResult>
    performSearch(const std::vector<std::string>& term, int topN);

    // 使用向量空间模型和余弦相似度计算文档得分
    double calculateCosineSimilarity(
        const std::vector<std::string>& queryTerms, int docid,
        const std::unordered_map<std::string, double>& queryVector);

    // 计算查询向量的TF-IDF权重（归一化）
    std::unordered_map<std::string, double>
    calculateQueryVector(const std::vector<std::string>& queryTerms);

    // 计算文档向量的TF-IDF权重（使用倒排索引中的归一化权重）
    std::unordered_map<std::string, double>
    getDocumentVector(int docid, const std::vector<std::string>& queryTerms);

    // 计算两个向量的点积
    double
    calculateDotProduct(const std::unordered_map<std::string, double>& vec1,
                        const std::unordered_map<std::string, double>& vec2);

    // 查找包含所有查询词的文档（交集策略）
    std::vector<int>
    findIntersectionDocuments(const std::vector<std::string>& terms);

    // 生成网页摘要
    std::string generateSummary(const std::string& content,
                                const std::vector<std::string>& terms);

    // 词汇标准化
    std::string normalizeQuery(const std::string& query);

    // 高亮关键词（在摘要中用特殊标记包围查询词）
    std::string highlightKeywords(const std::string& text,
                                  const std::vector<std::string>& terms);

    // 使用cppjieba进行中文分词
    std::vector<std::string> tokenizeQuery(const std::string& query);

    // UTF-8字符清理函数
    std::string cleanUtf8String(const std::string& input);

private:
    DataReaderManager* dataManager_;         // 数据读取管理器
    int maxSummaryLength_;                   // 最大摘要长度
    std::unique_ptr<cppjieba::Jieba> jieba_; // jieba分词器
    std::unique_ptr<StopWordsManager> stopWordsManager_; // 停用词管理器
    
    // 缓存系统
    mutable LRUCache<SearchCacheKey, std::vector<SearchResult>> searchCache_;
};

// 搜索响应包装器（用于TLV协议传输）
struct SearchResponse {
    std::string query;                 // 原始查询词
    std::vector<SearchResult> results; // 搜索结果列表
    int total;                         // 总结果数量
    int timestamp;                     // 时间戳

    // 转换为JSON，带错误处理
    nlohmann::json toJson() const {
        try {
            nlohmann::json jsonArray = nlohmann::json::array();
            
            // 逐个处理搜索结果，确保单个结果的错误不会影响整体
            for (const auto& result : results) {
                try {
                    jsonArray.push_back(result.toJson());
                } catch (const std::exception& e) {
                    // 如果单个结果序列化失败，添加一个错误占位符
                    jsonArray.push_back(nlohmann::json{
                        {"docid", result.docid},
                        {"title", "结果处理错误"},
                        {"url", "#"},
                        {"summary", "此结果无法正确显示"},
                        {"score", result.score}
                    });
                }
            }

            return nlohmann::json{{"query", query},
                                 {"results", jsonArray},
                                 {"total", static_cast<int>(jsonArray.size())},
                                 {"timestamp", timestamp}};
        } catch (const std::exception& e) {
            // 如果整体JSON序列化失败，返回最小化的安全JSON
            nlohmann::json safeJson;
            safeJson["query"] = query;
            safeJson["results"] = nlohmann::json::array();
            safeJson["total"] = 0;
            safeJson["timestamp"] = timestamp;
            safeJson["error"] = "搜索结果处理错误";
            return safeJson;
        }
    }
};
