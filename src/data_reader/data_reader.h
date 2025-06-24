#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// 词典项结构
struct DictionaryEntry {
    std::string word;
    int frequency;

    DictionaryEntry(const std::string& w, int freq)
        : word{w}
        , frequency{freq} {}
};

// 索引项结构
struct IndexEntry {
    std::string character;        // 中文字符或英文字母
    std::vector<int> lineNumbers; // 对应词典中的行号

    IndexEntry(const std::string& ch)
        : character{ch} {}
};

// 网页信息结构
struct WebPage {
    int docid;
    std::string link;
    std::string title;
    std::string content;

    WebPage(int id)
        : docid{id} {}
};

// 网页偏移信息结构
struct WebPageOffset {
    int docid;
    size_t offset;
    size_t length;

    WebPageOffset()
        : docid{0}
        , offset{0}
        , length{0} {}
    WebPageOffset(int id, size_t off, size_t len)
        : docid{id}
        , offset{off}
        , length{len} {}
};

// 倒排索引项结构
struct InvertedIndexEntry {
    std::string term;
    std::vector<std::pair<int, double>> docWeights; // <docid, weight>

    InvertedIndexEntry(const std::string& t)
        : term{t} {}
};

// 词典读取器基类
class DictionaryReader {
public:
    virtual ~DictionaryReader() = default;

    // 加载词典文件
    virtual bool loadDictionary(const std::string& dictPath) = 0;

    // 根据词获取词频
    virtual int getWordFrequency(const std::string& word) const = 0;

    // 获取所有词典项
    virtual std::vector<DictionaryEntry> getAllDictionaryEntries() const = 0;

    // 获取词典大小
    virtual size_t getDictionarySize() const = 0;
};

// 索引读取器基类
class IndexReader {
public:
    virtual ~IndexReader() = default;

    // 加载索引文件
    virtual bool loadIndex(const std::string& indexPath) = 0;

    // 根据字符/字母获取行号列表
    virtual std::vector<int>
    getLineNumbers(const std::string& character) const = 0;

    // 获取所有索引项
    virtual std::vector<IndexEntry> getAllIndexEntries() const = 0;
};

// 中文词典读取器
class ChineseDictionaryReader : public DictionaryReader {
public:
    ChineseDictionaryReader() = default;
    explicit ChineseDictionaryReader(const std::string& dictPath) {
        loadDictionary(dictPath);
    }
    
    bool loadDictionary(const std::string& dictPath) override;
    int getWordFrequency(const std::string& word) const override;
    std::vector<DictionaryEntry> getAllDictionaryEntries() const override;
    size_t getDictionarySize() const override;

private:
    std::unordered_map<std::string, int> wordFreqMap_; // 词频映射
    std::vector<DictionaryEntry> allEntries_;          // 所有词典项
};

// 英文词典读取器
class EnglishDictionaryReader : public DictionaryReader {
public:
    EnglishDictionaryReader() = default;
    explicit EnglishDictionaryReader(const std::string& dictPath) {
        loadDictionary(dictPath);
    }
    
    bool loadDictionary(const std::string& dictPath) override;
    int getWordFrequency(const std::string& word) const override;
    std::vector<DictionaryEntry> getAllDictionaryEntries() const override;
    size_t getDictionarySize() const override;

private:
    std::unordered_map<std::string, int> wordFreqMap_; // 词频映射
    std::vector<DictionaryEntry> allEntries_;          // 所有词典项
};

// 中文索引读取器
class ChineseIndexReader : public IndexReader {
public:
    ChineseIndexReader() = default;
    explicit ChineseIndexReader(const std::string& indexPath) {
        loadIndex(indexPath);
    }
    
    bool loadIndex(const std::string& indexPath) override;
    std::vector<int>
    getLineNumbers(const std::string& character) const override;
    std::vector<IndexEntry> getAllIndexEntries() const override;

private:
    std::unordered_map<std::string, std::vector<int>> indexMap_; // 索引映射
    std::vector<IndexEntry> allEntries_;                         // 所有索引项
};

// 英文索引读取器
class EnglishIndexReader : public IndexReader {
public:
    EnglishIndexReader() = default;
    explicit EnglishIndexReader(const std::string& indexPath) {
        loadIndex(indexPath);
    }
    
    bool loadIndex(const std::string& indexPath) override;
    std::vector<int>
    getLineNumbers(const std::string& character) const override;
    std::vector<IndexEntry> getAllIndexEntries() const override;

private:
    std::unordered_map<char, std::vector<int>> indexMap_; // 索引映射（按字符）
    std::vector<IndexEntry> allEntries_;                  // 所有索引项
};

// 网页库读取器 - 统一管理网页数据、偏移信息和倒排索引
class WebPageLibraryReader {
public:
    // 构造函数
    WebPageLibraryReader() = default;
    WebPageLibraryReader(const std::string& webpagesPath, 
                        const std::string& offsetPath, 
                        const std::string& invertedIndexPath) {
        initialize(offsetPath, webpagesPath, invertedIndexPath);
    }
    ~WebPageLibraryReader() = default;

    // 初始化：加载所有相关文件
    bool initialize(const std::string& offsetPath,
                    const std::string& webpagesPath,
                    const std::string& invertedIndexPath);

    // 倒排索引相关

    // 根据词获取文档id和权重
    std::vector<std::pair<int, double>>
    getDocuments(const std::string& term) const;

    // 判断词是否存在
    bool hasTerm(const std::string& term) const;

    // 获取倒排索引大小
    size_t getIndexSize() const;

    // 网页数据相关

    // 根据文档id获取单个网页
    std::unique_ptr<WebPage> getWebPage(int docid) const;

    // 根据文档id列表获取多个网页
    std::vector<std::unique_ptr<WebPage>>
    getWebPages(const std::vector<int>& docids) const;

    // 获取所有文档id
    std::vector<int> getAllDocIds() const;

    // 获取网页数量
    size_t getWebPageCount() const;

private:
    // 倒排索引：词汇 -> (文档ID, 权重)列表
    std::unordered_map<std::string, std::vector<std::pair<int, double>>>
        invertedIndex_;

    // 网页偏移信息：文档ID -> 偏移信息
    std::unordered_map<int, WebPageOffset> offsetMap_;

    // 网页文件路径
    std::string webpagesFilePath_;

    // 辅助函数
    std::unique_ptr<WebPage> parseWebPageXML(const std::string& xmlContent,
                                             int docid) const;
    bool loadInvertedIndex(const std::string& invertedIndexPath);
    bool loadOffsets(const std::string& offsetPath);
};

// 数据读取器管理类
class DataReaderManager {
public:
    DataReaderManager();
    ~DataReaderManager() = default;

    // 初始化所有数据读取器
    bool initialize(const std::string& dataDir);

    // 获取各种读取器
    ChineseDictionaryReader* getChineseDictionaryReader() const;
    EnglishDictionaryReader* getEnglishDictionaryReader() const;
    ChineseIndexReader* getChineseIndexReader() const;
    EnglishIndexReader* getEnglishIndexReader() const;
    WebPageLibraryReader* getWebPageLibraryReader() const;
    
    // 设置各种读取器
    void setChineseDictionaryReader(std::unique_ptr<ChineseDictionaryReader> reader);
    void setEnglishDictionaryReader(std::unique_ptr<EnglishDictionaryReader> reader);
    void setChineseIndexReader(std::unique_ptr<ChineseIndexReader> reader);
    void setEnglishIndexReader(std::unique_ptr<EnglishIndexReader> reader);
    void setWebPageLibraryReader(std::unique_ptr<WebPageLibraryReader> reader);

private:
    std::unique_ptr<ChineseDictionaryReader> chineseDictReader_;
    std::unique_ptr<EnglishDictionaryReader> englishDictReader_;
    std::unique_ptr<ChineseIndexReader> chineseIndexReader_;
    std::unique_ptr<EnglishIndexReader> englishIndexReader_;
    std::unique_ptr<WebPageLibraryReader> webPageLibraryReader_;
};
 