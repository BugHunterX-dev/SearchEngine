#include "data_reader.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::istringstream;
using std::make_unique;
using std::pair;
using std::regex;
using std::regex_search;
using std::smatch;
using std::string;
using std::unique_ptr;
using std::vector;

// ============ ChineseDictionaryReader 实现 ============
bool ChineseDictionaryReader::loadDictionary(const string& dictPath) {
    ifstream ifs{dictPath};
    if (!ifs.is_open()) {
        cerr << "无法打开中文词典文件: " << dictPath << endl;
        return false;
    }

    wordFreqMap_.clear();
    allEntries_.clear();

    string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析格式：词 频次
        istringstream iss{line};
        string word;
        int frequency;

        if (iss >> word >> frequency) {
            wordFreqMap_[word] = frequency;
            allEntries_.emplace_back(word, frequency);
        }
    }

    ifs.close();
    cout << "中文词典加载完成，共 " << allEntries_.size() << " 个词汇" << endl;
    return true;
}

int ChineseDictionaryReader::getWordFrequency(const string& word) const {
    auto it = wordFreqMap_.find(word);
    return (it != wordFreqMap_.end()) ? it->second : 0;
}

vector<DictionaryEntry>
ChineseDictionaryReader::getAllDictionaryEntries() const {
    return allEntries_;
}

size_t ChineseDictionaryReader::getDictionarySize() const {
    return allEntries_.size();
}

// ============ EnglishDictionaryReader 实现 ============
bool EnglishDictionaryReader::loadDictionary(const string& dictPath) {
    ifstream ifs{dictPath};
    if (!ifs.is_open()) {
        cerr << "无法打开英文词典文件: " << dictPath << endl;
        return false;
    }
    wordFreqMap_.clear();
    allEntries_.clear();
    string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析格式：词 频次
        istringstream iss{line};
        string word;
        int frequency;

        if (iss >> word >> frequency) {
            wordFreqMap_[word] = frequency;
            allEntries_.emplace_back(word, frequency);
        }
    }

    ifs.close();
    cout << "英文词典加载完成，共 " << allEntries_.size() << " 个词汇" << endl;
    return true;
}

int EnglishDictionaryReader::getWordFrequency(const string& word) const {
    auto it = wordFreqMap_.find(word);
    return (it != wordFreqMap_.end()) ? it->second : 0;
}

vector<DictionaryEntry>
EnglishDictionaryReader::getAllDictionaryEntries() const {
    return allEntries_;
}

size_t EnglishDictionaryReader::getDictionarySize() const {
    return allEntries_.size();
}

// ============ ChineseIndexReader 实现 ============
bool ChineseIndexReader::loadIndex(const string& indexPath) {
    ifstream ifs{indexPath};
    if (!ifs.is_open()) {
        cerr << "无法打开中文索引文件: " << indexPath << endl;
        return false;
    }

    indexMap_.clear();
    allEntries_.clear();

    string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析格式： 字符 行号1 行号2...
        istringstream iss{line};
        string character;

        if (iss >> character) {
            vector<int> lineNumbers;
            int lineNum;
            while (iss >> lineNum) {
                lineNumbers.push_back(lineNum);
            }

            indexMap_[character] = lineNumbers;
            IndexEntry entry(character);
            entry.lineNumbers = lineNumbers;
            allEntries_.push_back(entry);
        }
    }

    ifs.close();
    cout << "中文索引加载完成，共 " << allEntries_.size() << " 个字符索引"
         << endl;
    return true;
}

vector<int> ChineseIndexReader::getLineNumbers(const string& character) const {
    auto it = indexMap_.find(character);
    return (it != indexMap_.end()) ? it->second : vector<int>();
}

vector<IndexEntry> ChineseIndexReader::getAllIndexEntries() const {
    return allEntries_;
}

// ============ EnglishIndexReader 实现 ============
bool EnglishIndexReader::loadIndex(const string& indexPath) {
    ifstream ifs{indexPath};
    if (!ifs.is_open()) {
        cerr << "无法打开英文索引文件: " << indexPath << endl;
        return false;
    }

    indexMap_.clear();
    allEntries_.clear();

    string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析格式：字母 行号1 行号2 行号3 ...
        istringstream iss{line};
        char character;

        if (iss >> character) {
            vector<int> lineNumbers;
            int lineNum;
            while (iss >> lineNum) {
                lineNumbers.push_back(lineNum);
            }

            indexMap_[character] = lineNumbers;
            IndexEntry entry(string(1, character));
            entry.lineNumbers = lineNumbers;
            allEntries_.push_back(entry);
        }
    }

    ifs.close();
    cout << "英文索引加载完成，共 " << allEntries_.size() << " 个字母索引"
         << endl;
    return true;
}

vector<int> EnglishIndexReader::getLineNumbers(const string& character) const {
    if (character.length() != 1)
        return vector<int>();

    char ch = character[0];
    auto it = indexMap_.find(ch);
    return (it != indexMap_.end()) ? it->second : vector<int>();
}

vector<IndexEntry> EnglishIndexReader::getAllIndexEntries() const {
    return allEntries_;
}

// ============ WebPageLibraryReader 实现 ============
bool WebPageLibraryReader::initialize(const string& offsetPath,
                                      const string& webpagesPath,
                                      const string& invertedIndexPath) {
    cout << "开始初始化网页库读取器..." << endl;

    bool success{true};

    // 1. 加载倒排索引
    success &= loadInvertedIndex(invertedIndexPath);

    // 2. 加载偏移信息
    success &= loadOffsets(offsetPath);

    // 3. 保存网页文件路径，用于按需读取
    webpagesFilePath_ = webpagesPath;

    if (success) {
        cout << "网页库读取器初始化成功！" << endl;
        cout << "- 倒排索引: " << invertedIndex_.size() << " 个词汇" << endl;
        cout << "- 网页偏移: " << offsetMap_.size() << " 个网页" << endl;
    } else {
        cout << "网页库读取器初始化失败！" << endl;
    }

    return success;
}

bool WebPageLibraryReader::loadInvertedIndex(const string& invertedIndexPath) {
    ifstream ifs{invertedIndexPath};
    if (!ifs.is_open()) {
        cerr << "无法打开倒排索引文件: " << invertedIndexPath << endl;
        return false;
    }

    invertedIndex_.clear();

    string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析格式：词 docid1 权重1 docid2 权重2 ...
        istringstream iss{line};
        string term;

        if (iss >> term) {
            vector<pair<int, double>> docWeights;
            int docid;
            double weight;

            while (iss >> docid >> weight) {
                docWeights.emplace_back(docid, weight);
            }

            invertedIndex_[term] = docWeights;
        }
    }

    ifs.close();
    cout << "倒排索引加载完成，共 " << invertedIndex_.size() << " 个词汇"
         << endl;
    return true;
}

bool WebPageLibraryReader::loadOffsets(const string& offsetPath) {
    ifstream ifs{offsetPath};
    if (!ifs.is_open()) {
        cerr << "无法打开偏移文件: " << offsetPath << endl;
        return false;
    }

    offsetMap_.clear();

    string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析格式：docid offset length
        istringstream iss{line};
        int docid;
        size_t offset, length;

        if (iss >> docid >> offset >> length) {
            offsetMap_[docid] = WebPageOffset(docid, offset, length);
        }
    }

    ifs.close();
    cout << "网页偏移信息加载完成，共 " << offsetMap_.size() << " 个网页"
         << endl;
    return true;
}

vector<pair<int, double>>
WebPageLibraryReader::getDocuments(const string& term) const {
    auto it = invertedIndex_.find(term);
    return (it != invertedIndex_.end()) ? it->second
                                        : vector<pair<int, double>>();
}

bool WebPageLibraryReader::hasTerm(const string& term) const {
    return invertedIndex_.find(term) != invertedIndex_.end();
}

size_t WebPageLibraryReader::getIndexSize() const {
    return invertedIndex_.size();
}

unique_ptr<WebPage> WebPageLibraryReader::getWebPage(int docid) const {
    auto it = offsetMap_.find(docid);
    if (it == offsetMap_.end()) {
        return nullptr;
    }

    ifstream ifs{webpagesFilePath_};
    if (!ifs.is_open()) {
        cerr << "无法打开网页文件: " << webpagesFilePath_ << endl;
        return nullptr;
    }

    const WebPageOffset& offset{it->second};

    // 定位到指定位置
    ifs.seekg(offset.offset);

    // 读取指定长度的内容
    string content(offset.length, '\0');
    ifs.read(&content[0], offset.length);
    ifs.close();

    return parseWebPageXML(content, docid);
}

vector<unique_ptr<WebPage>>
WebPageLibraryReader::getWebPages(const vector<int>& docids) const {
    vector<unique_ptr<WebPage>> pages;
    pages.reserve(docids.size());

    for (int docid : docids) {
        auto page = getWebPage(docid);
        if (page) {
            pages.push_back(std::move(page));
        }
    }

    return pages;
}

vector<int> WebPageLibraryReader::getAllDocIds() const {
    vector<int> docids;
    docids.reserve(offsetMap_.size());

    // _表示忽略这个值
    for (const auto& [docid, _] : offsetMap_) {
        docids.push_back(docid);
    }

    sort(docids.begin(), docids.end());
    return docids;
}

size_t WebPageLibraryReader::getWebPageCount() const {
    return offsetMap_.size();
}

unique_ptr<WebPage>
WebPageLibraryReader::parseWebPageXML(const string& xmlContent,
                                      int docid) const {
    auto page{make_unique<WebPage>(docid)};

    // 使用正则表达式解析XML内容
    // 解析docid
    regex docidRegex{R"(<docid>([^<]+)</docid>)"};
    smatch match; // 保存正则匹配结果
    if (regex_search(xmlContent, match, docidRegex)) {
        // docid已经在构造函数设置
    }

    // 解析link
    regex linkRegex{R"(<link>([^<]+)</link>)"};
    if (regex_search(xmlContent, match, linkRegex)) {
        // match[1]：表示第一个“捕获组”（() 括起来的部分）匹配的内容
        page->link = match[1].str();
    }

    // 解析标题
    regex titleRegex(R"(<title>([^<]+)</title>)");
    if (regex_search(xmlContent, match, titleRegex)) {
        page->title = match[1].str();
    }

    // 解析内容
    regex contentRegex(R"(<content>([^<]+)</content>)");
    if (regex_search(xmlContent, match, contentRegex)) {
        page->content = match[1].str();
    }

    return page;
}

// ============ DataReaderManager 实现 ============
DataReaderManager::DataReaderManager()
    : chineseDictReader_{make_unique<ChineseDictionaryReader>()}
    , englishDictReader_{make_unique<EnglishDictionaryReader>()}
    , chineseIndexReader_{make_unique<ChineseIndexReader>()}
    , englishIndexReader_{make_unique<EnglishIndexReader>()}
    , webPageLibraryReader_{make_unique<WebPageLibraryReader>()} {}

bool DataReaderManager::initialize(const string& dataDir) {
    cout << "开始初始化数据读取器..." << endl;

    // 初始化成功标志，初始设为 true，后续每一步加载都会与它“与”一下
    // 如果任意一步失败，success 就会变成 false
    bool success = true;

    // 加载中文词典和索引
    // 布尔逻辑“连乘法”技巧，用于连续判断多个函数是否全部成功
    // 等价于success = success && chineseDictReader_->loadDictionary(...);
    success &= chineseDictReader_->loadDictionary(dataDir + "/dict_cn.dat");
    success &= chineseIndexReader_->loadIndex(dataDir + "/index_cn.dat");

    // 加载英文词典和索引
    success &= englishDictReader_->loadDictionary(dataDir + "/dict_en.dat");
    success &= englishIndexReader_->loadIndex(dataDir + "/index_en.dat");

    // 加载网页库（包含倒排索引、偏移信息和网页内容）
    success &= webPageLibraryReader_->initialize(
        dataDir + "/offsets.dat", dataDir + "/webpages.dat",
        dataDir + "/inverted_index.dat");

    if (success) {
        cout << "数据读取器初始化成功！" << endl;
    } else {
        cout << "数据读取器初始化失败！" << endl;
    }

    return success;
}

ChineseDictionaryReader* DataReaderManager::getChineseDictionaryReader() const {
    return chineseDictReader_.get();
}

EnglishDictionaryReader* DataReaderManager::getEnglishDictionaryReader() const {
    return englishDictReader_.get();
}

ChineseIndexReader* DataReaderManager::getChineseIndexReader() const {
    return chineseIndexReader_.get();
}

EnglishIndexReader* DataReaderManager::getEnglishIndexReader() const {
    return englishIndexReader_.get();
}

WebPageLibraryReader* DataReaderManager::getWebPageLibraryReader() const {
    return webPageLibraryReader_.get();
}

// 实现setter方法
void DataReaderManager::setChineseDictionaryReader(std::unique_ptr<ChineseDictionaryReader> reader) {
    chineseDictReader_ = std::move(reader);
}

void DataReaderManager::setEnglishDictionaryReader(std::unique_ptr<EnglishDictionaryReader> reader) {
    englishDictReader_ = std::move(reader);
}

void DataReaderManager::setChineseIndexReader(std::unique_ptr<ChineseIndexReader> reader) {
    chineseIndexReader_ = std::move(reader);
}

void DataReaderManager::setEnglishIndexReader(std::unique_ptr<EnglishIndexReader> reader) {
    englishIndexReader_ = std::move(reader);
}

void DataReaderManager::setWebPageLibraryReader(std::unique_ptr<WebPageLibraryReader> reader) {
    webPageLibraryReader_ = std::move(reader);
}
 