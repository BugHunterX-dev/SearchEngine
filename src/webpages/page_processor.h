#pragma once

#include "../keywords/common.h"
#include <cppjieba/Jieba.hpp>
#include <simhash/Simhasher.hpp>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

// 前向声明
namespace tinyxml2 {
class XMLElement;
class XMLDocument;
} // namespace tinyxml2

// 网页结构体
struct WebPage {
    int docid;
    std::string link;
    std::string title;
    std::string content;
};

// 网页偏移信息结构
struct WebPageOffset {
    int docid;     // 文档ID
    size_t offset; // 在文件中的起始位置
    size_t length; // 文档的长度
};

class PageProcessor {
public:
    PageProcessor(const std::string& stopwordsFile);
    ~PageProcessor() = default;

    // 解析单个XML文件
    std::vector<WebPage> parseXmlFile(const std::string& xmlPath);

    // 解析所有XML文件
    std::vector<WebPage> parseAllXmlFiles(const std::string& xmlDir);

    // 使用simhash对网页进行去重，返回去重后的网页集合
    void deduplicateWebPages(const std::vector<WebPage>& webpages, int topk,
                             int threshold);

    // 建立网页库和偏移库
    bool buildWebPagesAndOffsets(const std::string& outputPath,
                                 const std::string& offsetPath);
    
    // 建立倒排索引库
    bool buildInvertedIndex(const std::string& outputPath);

private:
    // 清理文本内容，去除XML标签和CDATA
    std::string cleanText(const std::string& rawText);

    // 去除CDATA标记
    std::string removeCDATA(const std::string& text);

    // 去除HTML标签
    std::string removeHtmlTags(const std::string& text);

    // 修剪空白字符
    std::string trim(const std::string& text);

    // 解析RSS格式的XML
    void parseRssFormat(tinyxml2::XMLElement* channel,
                        std::vector<WebPage>& webpages);

    // 倒排索引相关的辅助函数
    
    // 执行分词和停用词过滤
    // 功能：对所有文档进行中文分词，过滤停用词，统计词频和文档频率
    void tokenizationAndFiltering(
        std::unordered_map<std::string, std::unordered_map<int, int>>& termFrequency,
        std::unordered_map<std::string, int>& documentFrequency,
        std::unordered_map<int, int>& docTotalWords);
    
    // 计算TF-IDF权重
    // 功能：基于词频和文档频率计算每个词在每个文档中的TF-IDF权重值
    void calculateTfIdfWeights(
        const std::unordered_map<std::string, std::unordered_map<int, int>>& termFrequency,
        const std::unordered_map<std::string, int>& documentFrequency,
        std::unordered_map<std::string, std::unordered_map<int, double>>& tfidfWeights);
    
    // 执行L2归一化
    // 功能：对每个文档的TF-IDF权重向量进行L2归一化，消除文档长度影响
    void performL2Normalization(
        std::unordered_map<std::string, std::unordered_map<int, double>>& tfidfWeights);
    
    // 生成倒排索引文件
    // 功能：将归一化后的权重按Unicode排序，以"词 id 权重 id 权重..."格式写入文件
    bool generateInvertedIndexFile(
        const std::unordered_map<std::string, std::unordered_map<int, double>>& tfidfWeights,
        const std::string& outputPath);

    // 去重后的网页库
    std::vector<WebPage> uniquePages_;
    // simhash去重器
    std::unique_ptr<simhash::Simhasher> simhasher_;
    // 停用词管理器
    std::unique_ptr<StopWordsManager> stopWordsManager_;
    // jieba分词器
    std::unique_ptr<cppjieba::Jieba> jieba_;

    int nextDocId_ = 1;
};
