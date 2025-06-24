#pragma once

#include "../cache/lru_cache.h"
#include "../data_reader/data_reader.h"
#include <nlohmann/json.hpp>
#include <queue>
#include <set>
#include <string>
#include <vector>

// 候选词结构
struct Candidate {
    std::string word; // 候选词
    int editDistance; // 与查询词的编辑距离
    int frequency;    // 词频

    Candidate(const std::string& w = "", int ed = 0, int freq = 0)
        : word{w}
        , editDistance{ed}
        , frequency{freq} {}

    // 转换为JSON格式
    nlohmann::json toJson() const {
        return nlohmann::json{{"word", word},
                              {"editDistance", editDistance},
                              {"frequency", frequency}};
    }
};

// 候选词比较器（用于优先级队列）
struct CandidateComparator {
    bool operator()(const Candidate& a, const Candidate& b) const {
        // 优先比较编辑距离（距离小的优先）
        if (a.editDistance != b.editDistance) {
            return a.editDistance < b.editDistance;
        }

        // 2.编辑距离相同时，比较词频（频次高优先）
        if (a.frequency != b.frequency) {
            return a.frequency > b.frequency;
        }

        // 3.词频相同按字典序（字典序小的优先）
        return a.word < b.word;
    }
};

// 文本处理器类
class TextProcessor {
public:
    // UTF-8字符串分割函数（使用utf8cpp库）
    std::vector<std::string> utf8Split(const std::string& input);

    // 判断是否为中文字符
    bool isChinese(const std::string& text);

    // 判断是否为英文字符
    bool isEnglish(const std::string& text);
};

// 缓存键结构
struct RecommendationCacheKey {
    std::string query;
    int k;

    // 给哈希用
    bool operator==(const RecommendationCacheKey& other) const {
        return query == other.query && k == other.k;
    }

    // 给顺序容器用
    bool operator<(const RecommendationCacheKey& other) const {
        if (query != other.query) {
            return query < other.query;
        }
        return k < other.k;
    }
};

// 为缓存键提供hash函数
// 因为存放到unordered_map中，需要提供hash函数
namespace std {
template <>
struct hash<RecommendationCacheKey> {
    std::size_t operator()(const RecommendationCacheKey& key) const {
        return std::hash<std::string>()(key.query) ^
               (std::hash<int>()(key.k) << 1);
    }
};
} // namespace std

// 编辑距离缓存键
struct EditDistanceCacheKey {
    std::string word1;
    std::string word2;

    bool operator==(const EditDistanceCacheKey& other) const {
        return word1 == other.word1 && word2 == other.word2;
    }

    bool operator<(const EditDistanceCacheKey& other) const {
        if (word1 != other.word1) {
            return word1 < other.word1;
        }
        return word2 < other.word2;
    }
};

// 为编辑距离缓存键提供hash函数
namespace std {
template <>
struct hash<EditDistanceCacheKey> {
    std::size_t operator()(const EditDistanceCacheKey& key) const {
        return std::hash<std::string>()(key.word1) ^
               (std::hash<std::string>()(key.word2) << 1);
    }
};
} // namespace std

// 关键词推荐器主类
class KeywordRecommender {
public:
    explicit KeywordRecommender(DataReaderManager* dataManager);
    ~KeywordRecommender() = default;

    // 推荐接口
    std::vector<Candidate> recommend(const std::string& query, int k = 10);

    // 生成JSON格式的推荐结果
    nlohmann::json recommendToJson(const std::string& query, int k = 10);

    // 设置最大编辑距离或阈值
    void setMaxEditDistance(int maxDistance) {
        maxEditDistance_ = maxDistance;
    }

    // 缓存管理
    void setCacheCapacity(size_t recommendCacheCapacity = 500,
                          size_t editDistanceCacheCapacity = 2000) {
        recommendCache_.setCapacity(recommendCacheCapacity);
        editDistanceCache_.setCapacity(editDistanceCacheCapacity);
    }

    // 获取缓存统计信息
    struct CacheStatistics {
        LRUCache<RecommendationCacheKey, std::vector<Candidate>>::CacheStats
            recommendStats;
        LRUCache<EditDistanceCacheKey, int>::CacheStats editDistanceStats;
    };

    CacheStatistics getCacheStats() const {
        return {recommendCache_.getStats(), editDistanceCache_.getStats()};
    }

    // 清空缓存
    void clearCache() {
        recommendCache_.clear();
        editDistanceCache_.clear();
    }

private:
    // 核心算法：计算最小编辑距离
    int calculateEditDistance(const std::string& word1,
                              const std::string& words2);

    // 通过分字查找候选词
    std::vector<std::string> findCandidateWords(const std::string& query);

    // 从索引中获取字符对应的行号列表
    std::vector<int> getLineNumbersFromIndex(const std::string& character);

    // 根据行号从词典获取词汇
    std::string getWordFromDictionary(int lineNumber, bool isChinese);

    // 获取词汇频次
    int getWordFrequency(const std::string& word);

    // 选择前K个结果
    std::vector<Candidate> selectTopK(const std::vector<Candidate>& candidates,
                                      int k);

    // 判断输入主要语言类型
    bool isPrimarilyChinese(const std::string& input);

private:
    DataReaderManager* dataManager_; // 数据读取管理器
    TextProcessor textProcessor_;    // 文本处理器
    int maxEditDistance_;            // 最大编辑距离阈值

    // 缓存系统
    mutable LRUCache<RecommendationCacheKey, std::vector<Candidate>>
        recommendCache_;
    mutable LRUCache<EditDistanceCacheKey, int> editDistanceCache_;
};

// 推荐结果包装器（用于TLV协议传输）
struct RecommendationResponse {
    std::string query;                 // 原始查询词
    std::vector<Candidate> candidates; // 推荐候选词列表
    int timestamp;                     // 时间戳

    // 转换为JSON
    nlohmann::json toJson() const {
        // 构造 candidates 数组
        nlohmann::json candidateArray = nlohmann::json::array();
        for (const auto& candidate : candidates) {
            candidateArray.push_back(candidate.toJson());
        }

        return nlohmann::json{{"query", query},
                              {"timestamp", timestamp},
                              {"candidates", candidateArray}};
    }
};