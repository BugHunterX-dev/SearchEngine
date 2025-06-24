#include "chinese_tokenizer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::make_unique;
using std::ofstream;
using std::pair;
using std::string;
using std::unordered_set;
using std::vector;

//=============================================================================
// 中文文本预处理方法实现
//=============================================================================

bool ChineseTextPreprocessor::isValidChineseWord(const string& word) {
    if (word.empty()) {
        return false;
    }

    // 检查是否包含中文字符
    auto it{word.begin()};
    while (it != word.end()) {
        // utf8::next会返回当前it指向的utf8字符的码点
        // 并将it移动到下一个utf8字符的开始位置
        uint32_t codepoint{utf8::next(it, word.end())};
        // 检查是否是中文字符 (U+4E00 到 U+9FFF)
        if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
            return true;
        }
    }

    return false;
}

string ChineseTextPreprocessor::normalizeChineseText(const string& text) {
    string result;
    auto it{text.begin()};
    while (it != text.end()) {
        uint32_t codepoint{utf8::next(it, text.end())};

        // 只保留中文字符和空格
        if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) || // 中文字符
            codepoint == 0x0020) {                          // 空格
            utf8::append(codepoint, std::back_inserter(result));
        } else {
            // 将其他字符替换为空格
            result += " ";
        }
    }

    return result;
}

//=============================================================================
// 中文分词器实现
//=============================================================================

ChineseTokenizer::ChineseTokenizer(const string& stopwordsFile)
    : stopWordsManager_{make_unique<StopWordsManager>(stopwordsFile)}
    , jieba_{make_unique<cppjieba::Jieba>()}
    , totalWords_{0}
    , validWords_{0}
    , stopWordsFiltered_{0}
    , processedFiles_{0} {}

bool ChineseTokenizer::processDirectory(const string& inputDirectory,
                                        const string& dictFile,
                                        const string& indexFile,
                                        const string& fileExtension) {
    // 检查目录是否存在
    if (!DirectoryUtils::isDirectory(inputDirectory)) {
        cerr << "[ChineseTokenizer] " << inputDirectory
             << " is not a valid directory path" << endl;
        return false;
    }

    // 获取目录下所有指定扩展名文件
    vector<string> files =
        DirectoryUtils::getFilesInDirectory(inputDirectory, fileExtension);

    if (files.empty()) {
        cerr << "[ChineseTokenizer] No files found in " << inputDirectory
             << endl;
        return false;
    }

    cout << "[ChineseTokenizer] Found " << files.size()
         << " files in directory " << inputDirectory << endl;

    // 调用现有的多文件处理方法
    return processFiles(files, dictFile, indexFile);
}

bool ChineseTokenizer::processFiles(const vector<string>& inputFiles,
                                    const string& dictFile,
                                    const string& indexFile) {
    resetStatistics();

    cout << "[ChineseTokenizer] Starting to process " << inputFiles.size()
         << " files..." << endl;

    // 处理所有文件，统计词频
    for (const auto& filePath : inputFiles) {
        cout << "[ChineseTokenizer] Processing file: " << filePath << endl;
        if (!processFile(filePath)) {
            cerr << "[ChineseTokenizer] Failed to process file: " << filePath
                 << endl;
            return false;
        }
        processedFiles_++;
    }

    if (wordFrequency_.empty()) {
        cerr << "[ChineseTokenizer] No valid words extracted" << endl;
        return false;
    }

    cout << "[ChineseTokenizer] Word frequency counting completed, building "
            "character index..."
         << endl;

    buildCharacterIndex();

    cout << "[ChineseTokenizer] Saving dictionary file: " << dictFile << endl;
    if (!saveDictionary(dictFile)) {
        return false;
    }

    cout << "[ChineseTokenizer] Saving index file: " << indexFile << endl;
    if (!saveIndex(indexFile)) {
        return false;
    }

    cout << "[ChineseTokenizer] Processing completed successfully" << endl;

    return true;
}

bool ChineseTokenizer::processFile(const string& filePath) {
    ifstream ifs(filePath);
    if (!ifs.is_open()) {
        cerr << "[ChineseTokenizer] Cannot open file: " << filePath << endl;
        return false;
    }

    string line;
    size_t lineCount{0};
    while (std::getline(ifs, line)) {
        ++lineCount;
        if (lineCount % 5000 == 0) {
            cout << "  [ChineseTokenizer] Processed " << lineCount
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

vector<string> ChineseTokenizer::processLine(const string& line) {
    vector<string> result;

    // 1.预处理文本
    string normalizedText = ChineseTextPreprocessor::normalizeChineseText(line);

    // 2.使用jieba进行分词
    auto words = segmentText(normalizedText);

    // 3.处理每个词
    for (const auto& word : words) {
        ++totalWords_;

        // 检查是否为有效中文词
        if (ChineseTextPreprocessor::isValidChineseWord(word)) {
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

vector<string> ChineseTokenizer::segmentText(const string& text) {
    vector<string> words;
    jieba_->Cut(text, words);
    return words;
}

void ChineseTokenizer::addWord(const string& word) {
    if (!word.empty()) {
        wordFrequency_[word]++;
    }
}

void ChineseTokenizer::buildCharacterIndex() {
    // 按Unicode编码顺序排序
    vector<pair<string, int>> sortedWords;
    for (const auto& pair : wordFrequency_) {
        sortedWords.push_back(pair);
    }

    sort(sortedWords.begin(), sortedWords.end());

    // 根据排序后的词典顺序构建索引
    int lineNumber = 1;
    for (const auto& pair : sortedWords) {
        const string& word = pair.first;

        // 提取词语中的所有汉字字符
        vector<string> characters = extractAllCharacters(word);

        // 为每个汉字字符建立索引，避免重复
        unordered_set<string> uniqueChars;
        for (const string& character : characters) {
            if (!character.empty() &&
                uniqueChars.find(character) == uniqueChars.end()) {
                // uniqueChars.find(character) == uniqueChars.end()表示没找到
                uniqueChars.insert(character);
                characterIndex_[character].push_back(lineNumber);
            }
        }

        lineNumber++;
    }
}

vector<string> ChineseTokenizer::extractAllCharacters(const string& word) {
    vector<string> characters;
    if (word.empty()) {
        return characters;
    }

    auto it{word.begin()};
    while (it != word.end()) {
        auto charBegin = it;
        uint32_t codepoint{utf8::next(it, word.end())};

        // 检查是否是中文字符（U+4E00 ~ U+9FFF）
        if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
            string character{charBegin, it}; // 用原起点和新it取字符
            characters.push_back(character);
        }
    }

    return characters;
}

bool ChineseTokenizer::saveDictionary(const string& dictFile) {
    ofstream ofs(dictFile);
    if (!ofs.is_open()) {
        cerr << "[ChineseTokenizer] Cannot create dictionary file: " << dictFile
             << endl;
        return false;
    }

    // 按Unicode编码顺序排序
    vector<pair<string, int>> sortedWords;
    for (const auto& pair : wordFrequency_) {
        sortedWords.push_back(pair);
    }

    std::sort(sortedWords.begin(), sortedWords.end());

    for (const auto& pair : sortedWords) {
        ofs << pair.first << " " << pair.second << '\n';
    }

    ofs.close();
    cout << "[ChineseTokenizer] Chinese dictionary saved to: " << dictFile
         << endl;
    return true;
}

bool ChineseTokenizer::saveIndex(const string& indexFile) {
    ofstream ofs(indexFile);
    if (!ofs.is_open()) {
        cerr << "[ChineseTokenizer] Cannot create index file: " << indexFile
             << endl;
        return false;
    }

    for (const auto& pair : characterIndex_) {
        ofs << pair.first;
        for (int lineNum : pair.second) {
            ofs << " " << lineNum;
        }
        ofs << '\n';
    }

    ofs.close();
    cout << "[ChineseTokenizer] Chinese index saved to: " << indexFile << endl;

    return true;
}

void ChineseTokenizer::printStatistics() const {
    cout << "\n=== Chinese Tokenizer Statistics ===" << endl;
    cout << "Files processed: " << processedFiles_ << endl;
    cout << "Total words processed: " << totalWords_ << endl;
    cout << "Valid Chinese words: " << validWords_ << endl;
    cout << "Unique words in dictionary: " << wordFrequency_.size() << endl;
    cout << "Stopwords filtered: " << stopWordsFiltered_ << endl;
    cout << "Invalid words: "
         << (totalWords_ - validWords_ - stopWordsFiltered_) << endl;
    cout << "Character indices: " << characterIndex_.size() << endl;
    cout << "=====================================" << endl;
}

void ChineseTokenizer::resetStatistics() {
    totalWords_ = 0;
    validWords_ = 0;
    stopWordsFiltered_ = 0;
    processedFiles_ = 0;
    wordFrequency_.clear();
    characterIndex_.clear();
}