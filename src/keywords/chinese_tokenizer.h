#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <fstream>
#include <memory>

// 集成第三方库
#include <cppjieba/Jieba.hpp>
#include <utf8cpp/utf8.h>

// 包含公共头文件
#include "common.h"

// 中文文本预处理器类
class ChineseTextPreprocessor {
public:
    // 检查是否为有效中文词
    static bool isValidChineseWord(const std::string& word);  
    // 规范化中文文本
    static std::string normalizeChineseText(const std::string& text);  
};

// 中文分词器类
class ChineseTokenizer {
public:
    explicit ChineseTokenizer(const std::string& stopwordsFile);
    
    // 处理指定目录并生成词典和索引文件
    bool processDirectory(const std::string& inputDirectory, 
                         const std::string& dictFile = "dict_cn.dat",
                         const std::string& indexFile = "index_cn.dat",
                         const std::string& fileExtension = ".txt");
    
    // 处理文件列表并生成词典和索引文件
    bool processFiles(const std::vector<std::string>& inputFiles,
                     const std::string& dictFile = "dict_cn.dat", 
                     const std::string& indexFile = "index_cn.dat");
    
    // 获取统计信息
    size_t getTotalWords() const { return totalWords_; }
    size_t getUniqueWords() const { return wordFrequency_.size(); }
    size_t getValidWords() const { return validWords_; }
    size_t getStopWordsFiltered() const { return stopWordsFiltered_; }
    size_t getProcessedFiles() const { return processedFiles_; }
    
    void printStatistics() const;  // 打印统计信息

private:
    std::unique_ptr<StopWordsManager> stopWordsManager_;  // 停用词管理器
    std::unique_ptr<cppjieba::Jieba> jieba_;  // jieba分词器
    
    // 核心数据结构
    std::unordered_map<std::string, int> wordFrequency_;     // 词频统计：词语 -> 频次
    std::map<std::string, std::vector<int>> characterIndex_;    // 汉字索引：首字 -> 词典行号列表
    
    // 统计信息
    size_t totalWords_;         // 总词数
    size_t validWords_;         // 有效词数
    size_t stopWordsFiltered_;  // 过滤的停用词数
    size_t processedFiles_;     // 已处理文件数
    
    // 核心处理方法
    bool processFile(const std::string& filePath);  // 处理单个文件
    std::vector<std::string> processLine(const std::string& line);  // 处理单行文本
    void addWord(const std::string& word);  // 添加词语到词频统计
    
    // 分词方法
    std::vector<std::string> segmentText(const std::string& text);  // 中文分词
    
    // 索引构建方法
    void buildCharacterIndex();  // 构建汉字索引
    
    // 文件输出方法
    bool saveDictionary(const std::string& dictFile);  // 保存词典文件
    bool saveIndex(const std::string& indexFile);  // 保存索引文件

    // 重置统计信息
    void resetStatistics();  
     // 提取词语中的所有汉字字符
    std::vector<std::string> extractAllCharacters(const std::string& word); 
}; 