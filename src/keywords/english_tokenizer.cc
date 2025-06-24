#include "english_tokenizer.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::istringstream;
using std::ofstream;
using std::pair;
using std::string;
using std::unordered_set;
using std::vector;

// ============ TextPreprocessor类实现 ===============

// 转换为小写
string EnglishTextPreprocessor::toLowerCase(const string& text) {
   string result{text};
    std::transform(result.begin(), result.end(), result.begin(), tolower);
    return result;
}

// 移除标点符号
string EnglishTextPreprocessor::removePunctuation(const string& text) {
    string result;

    for (char c : text) {
        if (isalpha(c) || isspace(c)) {
            result += c;
        } else {
            result += ' '; // 将标点符号替换为空格
        }
    }

    return result;
}

// 检查是否为有效单词
bool EnglishTextPreprocessor::isValidWord(const string& word) {
    // 过滤长度小于2的单词
    if (word.length() < 2) {
        return false;
    }

    // 检查是否全部由字母组成
    for (char c : word) {
        if (!isalpha(c)) {
            return false;
        }
    }

    return true;
}

// 分割为单词
vector<string> EnglishTextPreprocessor::splitIntoWords(const string& text) {
    vector<string> words;
    istringstream iss{text};
    string word;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

// ============ EnglishTokenizer类实现 ===============

EnglishTokenizer::EnglishTokenizer(const string& stopwordsFile)
    : stopWordsManager_{std::make_unique<StopWordsManager>(stopwordsFile)}
    , totalWords_{0}
    , validWords_{0}
    , stopWordsFiltered_{0}
    , processedFiles_{0} {}

// 处理指定目录并生成词典和索引文件
bool EnglishTokenizer::processDirectory(const string& inputDirectory,
                                        const string& dictFile,
                                        const string& indexFile,
                                        const string& fileExtension) {
    // 检查目录是否存在
    if (!DirectoryUtils::isDirectory(inputDirectory)) {
        cerr << "[EnglishTokenizer] " << inputDirectory
             << " is not a valid directory path" << endl;
        return false;
    }

    // 获取目录下所有指定扩展名文件
    auto files{
        DirectoryUtils::getFilesInDirectory(inputDirectory, fileExtension)};

    if (files.empty()) {
        cerr << "[EnglishTokenizer] No files found in " << inputDirectory
             << endl;
        return false;
    }

    cout << "[EnglishTokenizer] Found " << files.size()
         << " files in directory " << inputDirectory << endl;

    // 调用现有的多文件处理方法
    return processFiles(files, dictFile, indexFile);
}

// 处理指定文件并生成词典和索引文件
bool EnglishTokenizer::processFiles(const std::vector<std::string>& inputFiles,
                                    const std::string& dictFile,
                                    const std::string& indexFile) {
    resetStatistics();

    cout << "[EnglishTokenizer] Starting to process " << inputFiles.size()
         << " files..." << endl;

    // 处理所有文件，统计词频
    for (const auto& filePath : inputFiles) {
        cout << "[EnglishTokenizer] Processing file: " << filePath << endl;
        if (!processFile(filePath)) {
            cerr << "[EnglishTokenizer] Failed to process file: " << filePath
                 << endl;
            continue;
        }

        ++processedFiles_;
    }

    if (wordFrequency_.empty()) {
        cerr << "[EnglishTokenizer] No valid words extracted" << endl;
        return false;
    }

    cout << "[EnglishTokenizer] Word frequency counting completed, building "
            "letter index..."
         << endl;

    // 构建字母索引
    buildLetterIndex();

    cout << "[EnglishTokenizer] Saving dictionary file: " << dictFile << endl;
    if (!saveDictionary(dictFile)) {
        return false;
    }

    cout << "[EnglishTokenizer] Saving index file: " << indexFile << endl;
    if (!saveIndex(indexFile)) {
        return false;
    }

    cout << "[EnglishTokenizer] Processing completed successfully" << endl;

    return true;
}

// 处理文件
// 处理单个文件
bool EnglishTokenizer::processFile(const std::string& filePath) {
    ifstream ifs{filePath};
    if (!ifs.is_open()) {
        cerr << "[EnglishTokenizer] Failed to open file: " << filePath << endl;
        return false;
    }

    string line;
    size_t lineCount{0};
    while (std::getline(ifs, line)) {
        ++lineCount;
        if (lineCount % 5000 == 0) {
            cout << "  [EnglishTokenizer] Processed " << lineCount
                 << " lines..." << endl;
        }

        if (!line.empty()) {
            auto words{processLine(line)};
            for (const auto& word : words) {
                addWord(word);
            }
        }
    }

    ifs.close();

    return true;
}

// 处理单行文本
vector<string> EnglishTokenizer::processLine(const string& line) {
    vector<string> result;

    // 1.转换为小写
    string processed{EnglishTextPreprocessor::toLowerCase(line)};

    // 2.移除标点符号
    processed = EnglishTextPreprocessor::removePunctuation(processed);

    // 3.分割成单词
    auto words{EnglishTextPreprocessor::splitIntoWords(processed)};

    // 4.过滤和处理每个单词
    for (const auto& word : words) {
        ++totalWords_;

        // 检查是否是有效单词
        if (EnglishTextPreprocessor::isValidWord(word)) {
            // 检查是否为停用词
            if (!stopWordsManager_->isStopWord(word)) {
                result.push_back(word);
                ++validWords_;
            } else {
                ++stopWordsFiltered_;
            }
        }
    }

    return result;
}

// 添加单词到词频统计
void EnglishTokenizer::addWord(const string& word) {
    if (!word.empty()) {
        wordFrequency_[word]++;
    }
}

// 构建字母索引
void EnglishTokenizer::buildLetterIndex() {
    // 将词汇按照字典序排序
    vector<pair<string, int>> sortedWords;
    for (const auto& pair : wordFrequency_) {
        sortedWords.push_back(pair);
    }

    std::sort(sortedWords.begin(), sortedWords.end());

    // 为每个字母建立索引
    // 每次循环可以将一行单词中的字母添加到索引中
    for (size_t i = 0; i < sortedWords.size(); ++i) {
        const string& word = sortedWords[i].first;
        int lineNumber = i + 1; // 词典文件中的行号，从1开始

        // 为单词中每个字母建立索引
        unordered_set<char> wordLetters;
        for (char c : word) {
            if (isalpha(c)) {
                wordLetters.insert(c);
            }
        }

        // 添加单词到字母索引中
        for (char letter : wordLetters) {
            letterIndex_[letter].push_back(lineNumber);
        }
    }
}

// 保存词典文件
bool EnglishTokenizer::saveDictionary(const string& dictFile) {
    ofstream ofs{dictFile};
    if (!ofs.is_open()) {
        cerr << "[EnglishTokenizer] Cannot create dictionary file: " << dictFile
             << endl;
        return false;
    }

    // 将词汇按照字典序排序
    vector<pair<string, int>> sortedWords;
    for (const auto& pair : wordFrequency_) {
        sortedWords.push_back(pair);
    }

    std::sort(sortedWords.begin(), sortedWords.end());

    // 写入词典文件
    for (const auto& pair : sortedWords) {
        ofs << pair.first << " " << pair.second << "\n";
    }

    ofs.close();
    cout << "[EnglishTokenizer] English dictionary saved to: " << dictFile
         << endl;

    return true;
}

// 保存索引文件
bool EnglishTokenizer::saveIndex(const string& indexFile) {
    ofstream ofs{indexFile};
    if (!ofs.is_open()) {
        cerr << "[EnglishTokenizer] Cannot create index file: " << indexFile
             << endl;
        return false;
    }

    for(const auto& pair:letterIndex_){
        ofs << pair.first;
        for(int lineNum:pair.second){
            ofs << " " << lineNum;
        }
        ofs << '\n';
    }

    ofs.close();
    cout << "[EnglishTokenizer] English index saved to: " << indexFile << endl;

    return true;
}

// 重置统计信息
void EnglishTokenizer::resetStatistics() {
    wordFrequency_.clear();
    letterIndex_.clear();
    totalWords_ = 0;
    validWords_ = 0;
    stopWordsFiltered_ = 0;
    processedFiles_ = 0;
}

void EnglishTokenizer::printStatistics() const {
    cout << "\n=== English Tokenization Statistics ===" << endl;

    cout << "Files processed: " << processedFiles_ << endl;
    cout << "Total words: " << totalWords_ << endl;
    cout << "Valid words: " << validWords_ << endl;
    cout << "Unique words: " << wordFrequency_.size() << endl;
    cout << "Stopwords filtered: " << stopWordsFiltered_ << endl;
    cout << "Invalid words: "
         << (totalWords_ - validWords_ - stopWordsFiltered_) << endl;
    cout << "Letter indices: " << letterIndex_.size() << endl;

    if (totalWords_ > 0) {
        double validRate = (double)validWords_ / totalWords_ * 100;
        double stopRate = (double)stopWordsFiltered_ / totalWords_ * 100;
        cout << "Valid word ratio: " << validRate << "%" << endl;
        cout << "Stopword ratio: " << stopRate << "%" << endl;
    }
    cout << "=====================================" << endl;
}