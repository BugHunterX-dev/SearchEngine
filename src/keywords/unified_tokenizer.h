#pragma once

#include <string>
#include <memory>

// 前置声明
class EnglishTokenizer;
class ChineseTokenizer;

// 统一分词器类 - 支持中英文混合
class UnifiedTokenizer {
public:
    explicit UnifiedTokenizer(const std::string& enStopwordsFile, 
                             const std::string& cnStopwordsFile);
    
    // 析构函数
    ~UnifiedTokenizer();
    
    // 处理指定目录并生成词典和索引文件
    bool processDirectories(const std::string& enDirectory, 
                           const std::string& cnDirectory,
                           const std::string& enDictFile = "dict_en.dat",
                           const std::string& enIndexFile = "index_en.dat",
                           const std::string& cnDictFile = "dict_cn.dat",
                           const std::string& cnIndexFile = "index_cn.dat");
    
    // 分别处理中英文目录
    bool processEnglishDirectory(const std::string& enDirectory,
                                const std::string& enDictFile = "dict_en.dat",
                                const std::string& enIndexFile = "index_en.dat");
                                
    bool processChineseDirectory(const std::string& cnDirectory,
                                const std::string& cnDictFile = "dict_cn.dat",
                                const std::string& cnIndexFile = "index_cn.dat");
    
    // 获取统计信息
    size_t getEnglishUniqueWords() const;
    size_t getChineseUniqueWords() const;
    size_t getTotalProcessedFiles() const;
    
    // 打印统计信息
    void printAllStatistics() const;
    void printEnglishStatistics() const;
    void printChineseStatistics() const;

private:
    std::unique_ptr<EnglishTokenizer> englishTokenizer_;
    std::unique_ptr<ChineseTokenizer> chineseTokenizer_;
}; 