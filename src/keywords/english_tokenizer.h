#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.h"

// 词汇信息结构
struct WordInfo {
    std::string word; // 单词
    int frequency;    // 词频
    
    WordInfo(const std::string& w, int freq = 1) 
        : word(w), frequency(freq) {}
};

// 文档位置信息结构体
struct DocumentPosition {
    int doc_id;                // 文档ID
    std::vector<int> positions; // 词在文档中的位置列表
    
    DocumentPosition(int id) : doc_id(id) {}
};

// 英文文本预处理类
class EnglishTextPreprocessor {
public:
    // 转换为小写
    static std::string toLowerCase(const std::string& text);
    // 移除标点符号
    static std::string removePunctuation(const std::string& text);
    // 检查是否为有效单词
    static bool isValidWord(const std::string& word);
    // 分割为单词
    static std::vector<std::string> splitIntoWords(const std::string& text);
};

// 英文分词器类
class EnglishTokenizer {
public:
    explicit EnglishTokenizer(const std::string& stopwordsFile);

    // 处理指定目录并生成词典和索引文件
    bool processDirectory(const std::string& inputDirectory,
                          const std::string& dictFile = "dict_en.dat",
                          const std::string& indexFile = "index_en.dat",
                          const std::string& fileExtension = ".txt");

    // 处理指定文件并生成词典和索引文件
    bool processFiles(const std::vector<std::string>& inputFiles,
                      const std::string& dictFile = "dict_en.dat",
                      const std::string& indexFile = "index_en.dat");

    // 获取统计信息
    size_t getTotalWords() const {
        return totalWords_;
    }
    size_t getUniqueWords() const {
        return wordFrequency_.size();
    }
    size_t getValidWords() const {
        return validWords_;
    }
    size_t getStopWordsFiltered() const {
        return stopWordsFiltered_;
    }
    size_t getProcessedFiles() const {
        return processedFiles_;
    }

    // 打印统计信息
    void printStatistics() const;

private:
    // 处理单个文件
    bool processFile(const std::string& filePath); 
    // 处理单行文本
    std::vector<std::string>
    processLine(const std::string& line); 
    // 添加单词到词频统计 
    void addWord(const std::string& word); 

    // 构建字母索引
    void buildLetterIndex();
   
   // 保存词典文件
    bool saveDictionary(const std::string& dictFile);
    // 保存索引文件 
    bool saveIndex(const std::string& indexFile);     

    // 重置统计信息
    void resetStatistics();

    std::unique_ptr<StopWordsManager> stopWordsManager_; // 停用词管理器

    // 词频统计,用unordered_map提高查找速度
    std::unordered_map<std::string, int> wordFrequency_;
    // 字母索引（字母->词典行号）
    std::map<char, std::vector<int>> letterIndex_;

    // 统计信息
    size_t totalWords_;
    size_t validWords_;
    size_t stopWordsFiltered_;
    size_t processedFiles_;
};


