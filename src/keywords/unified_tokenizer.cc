#include "unified_tokenizer.h"
#include "chinese_tokenizer.h"
#include "english_tokenizer.h"
#include <iostream>

using namespace std;

//=============================================================================
// 统一分词器实现
//=============================================================================

UnifiedTokenizer::UnifiedTokenizer(const std::string& enStopwordsFile,
                                   const std::string& cnStopwordsFile)
    : englishTokenizer_(std::make_unique<EnglishTokenizer>(enStopwordsFile))
    , chineseTokenizer_(std::make_unique<ChineseTokenizer>(cnStopwordsFile)) {

    std::cout << "=== Unified Tokenizer Initialized ===" << std::endl;
    std::cout << "English stopwords: " << enStopwordsFile << std::endl;
    std::cout << "Chinese stopwords: " << cnStopwordsFile << std::endl;
}

UnifiedTokenizer::~UnifiedTokenizer() {
    std::cout << "=== Unified Tokenizer Destroyed ===" << std::endl;
}

bool UnifiedTokenizer::processDirectories(const std::string& enDirectory,
                                          const std::string& cnDirectory,
                                          const std::string& enDictFile,
                                          const std::string& enIndexFile,
                                          const std::string& cnDictFile,
                                          const std::string& cnIndexFile) {
    std::cout << "=== Starting Unified Tokenization Process ===" << std::endl;

    // 处理英文文档
    std::cout << "\n--- Processing English Documents ---" << std::endl;
    bool enSuccess = englishTokenizer_->processDirectory(
        enDirectory, enDictFile, enIndexFile);

    // 处理中文文档
    std::cout << "\n--- Processing Chinese Documents ---" << std::endl;
    bool cnSuccess = chineseTokenizer_->processDirectory(
        cnDirectory, cnDictFile, cnIndexFile);

    if (enSuccess && cnSuccess) {
        std::cout << "\n=== All Processing Completed Successfully ==="
                  << std::endl;
    } else {
        std::cout << "\n=== Some Processing Failed ===" << std::endl;
        if (!enSuccess)
            std::cerr << "English processing failed!" << std::endl;
        if (!cnSuccess)
            std::cerr << "Chinese processing failed!" << std::endl;
    }

    return enSuccess && cnSuccess;
}

bool UnifiedTokenizer::processEnglishDirectory(const std::string& enDirectory,
                                               const std::string& enDictFile,
                                               const std::string& enIndexFile) {
    std::cout << "=== Processing English Documents Only ===" << std::endl;
    return englishTokenizer_->processDirectory(enDirectory, enDictFile,
                                               enIndexFile);
}

bool UnifiedTokenizer::processChineseDirectory(const std::string& cnDirectory,
                                               const std::string& cnDictFile,
                                               const std::string& cnIndexFile) {
    std::cout << "=== Processing Chinese Documents Only ===" << std::endl;
    return chineseTokenizer_->processDirectory(cnDirectory, cnDictFile,
                                               cnIndexFile);
}

size_t UnifiedTokenizer::getEnglishUniqueWords() const {
    return englishTokenizer_->getUniqueWords();
}

size_t UnifiedTokenizer::getChineseUniqueWords() const {
    return chineseTokenizer_->getUniqueWords();
}

size_t UnifiedTokenizer::getTotalProcessedFiles() const {
    return englishTokenizer_->getProcessedFiles() +
           chineseTokenizer_->getProcessedFiles();
}

void UnifiedTokenizer::printAllStatistics() const {
    std::cout << "\n=== Unified Tokenizer Complete Statistics ===" << std::endl;

    // 打印英文统计
    englishTokenizer_->printStatistics();

    // 打印中文统计
    chineseTokenizer_->printStatistics();

    // 打印综合统计
    std::cout << "\n=== Overall Summary ===" << std::endl;
    std::cout << "Total English unique words: " << getEnglishUniqueWords()
              << std::endl;
    std::cout << "Total Chinese unique words: " << getChineseUniqueWords()
              << std::endl;
    std::cout << "Total files processed: " << getTotalProcessedFiles()
              << std::endl;

    // 计算比例
    size_t totalWords = getEnglishUniqueWords() + getChineseUniqueWords();
    if (totalWords > 0) {
        double enPercentage =
            (double)getEnglishUniqueWords() / totalWords * 100;
        double cnPercentage =
            (double)getChineseUniqueWords() / totalWords * 100;
        std::cout << "English words ratio: " << enPercentage << "%"
                  << std::endl;
        std::cout << "Chinese words ratio: " << cnPercentage << "%"
                  << std::endl;
    }

    std::cout << "========================================" << std::endl;
}

void UnifiedTokenizer::printEnglishStatistics() const {
    std::cout << "\n=== English Only Statistics ===" << std::endl;
    englishTokenizer_->printStatistics();
}

void UnifiedTokenizer::printChineseStatistics() const {
    std::cout << "\n=== Chinese Only Statistics ===" << std::endl;
    chineseTokenizer_->printStatistics();
}