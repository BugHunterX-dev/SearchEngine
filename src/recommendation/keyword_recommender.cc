#include "keyword_recommender.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iostream>
#include <unordered_set>
#include <utf8cpp/utf8.h>

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::string;
using std::unordered_set;
using std::vector;

// ============ TextProcessor 实现 ============
// 将 UTF-8 字符串按"字符"（不是字节）分割成 vector<string>
vector<string> TextProcessor::utf8Split(const string& input) {
    vector<string> result;
    auto it = input.begin();
    while (it != input.end()) {
        auto start = it;
        utf8::next(it, input.end());    // it 会前进一个 UTF-8 字符
        result.emplace_back(start, it); // 截取这个字符,原地构造
    }
    return result;
}

bool TextProcessor::isChinese(const string& text) {
    if (text.empty())
        return false;

    auto it{text.begin()};
    uint32_t codepoint{utf8::next(it, text.end())};

    // 基本汉字范围：U+4E00 ~ U+9FFF
    return (codepoint >= 0x4E00 && codepoint <= 0x9FFF);
}

bool TextProcessor::isEnglish(const string& text) {
    return text.size() == 1 && isalpha(static_cast<unsigned char>(text[0]));
}

// ============ KeywordRecommender 实现 ============
KeywordRecommender::KeywordRecommender(DataReaderManager* dataManager)
    : dataManager_{dataManager}
    , maxEditDistance_{3}
    , recommendCache_(500)      // 推荐结果缓存，默认500个条目
    , editDistanceCache_(2000)  // 编辑距离缓存，默认2000个条目
{}

vector<Candidate> KeywordRecommender::recommend(const string& query, int k) {
    if (query.empty() || !dataManager_) {
        return {};
    }

    // 尝试从缓存获取结果
    RecommendationCacheKey cacheKey{query, k};
    vector<Candidate> cachedResult;
    if (recommendCache_.get(cacheKey, cachedResult)) {
        cout << "从缓存获取推荐结果，查询词: " << query << ", 返回数量: " << k << endl;
        return cachedResult;
    }

    cout << "开始推荐，查询词: " << query << ", 返回数量: " << k << endl;

    // 1.查找候选词
    auto candidateWords{findCandidateWords(query)};
    cout << "找到候选词数量：" << candidateWords.size() << endl;

    // 2.计算编辑距离或频次
    vector<Candidate> candidates;
    for (const auto& word : candidateWords) {
        int editDist{calculateEditDistance(query, word)};
        // 过滤编辑距离过大的词
        if (editDist <= maxEditDistance_) {
            int frequency{getWordFrequency(word)};
            candidates.emplace_back(word, editDist, frequency);
        }
    }

    cout << "过滤后候选词数量: " << candidates.size() << endl;

    // 3.返回前K个结果
    auto result = selectTopK(candidates, k);
    
    // 将结果存入缓存
    recommendCache_.put(cacheKey, result);
    
    return result;
}

nlohmann::json KeywordRecommender::recommendToJson(const string& query, int k) {
    auto candidates{recommend(query, k)};

    // 创建推荐结果对象
    RecommendationResponse resp;
    resp.query = query;
    resp.candidates = candidates;
    resp.timestamp = static_cast<int>(std::time(nullptr));

    return resp.toJson();
}

int KeywordRecommender::calculateEditDistance(const string& word1,
                                              const string& word2) {
    // 尝试从缓存获取编辑距离
    EditDistanceCacheKey cacheKey{word1, word2};
    int cachedDistance;
    if (editDistanceCache_.get(cacheKey, cachedDistance)) {
        return cachedDistance;
    }
    
    // 如果缓存中没有，尝试交换参数顺序查找（编辑距离是对称的）
    EditDistanceCacheKey reverseCacheKey{word2, word1};
    if (editDistanceCache_.get(reverseCacheKey, cachedDistance)) {
        return cachedDistance;
    }
    
    // 将字符串分解为UTF-8字符数组，然后计算编辑距离
    auto chars1 = textProcessor_.utf8Split(word1);
    auto chars2 = textProcessor_.utf8Split(word2);

    int m = chars1.size();
    int n = chars2.size();

    // 创建DP表
    vector<vector<int>> dp(m + 1, vector<int>(n + 1));

    // 初始化边界条件
    for (int i = 0; i <= m; ++i) {
        dp[i][0] = i; // 删除
    }
    for (int j = 0; j <= n; ++j) {
        dp[0][j] = j; // 插入
    }

    // 填充DP表
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int insert{dp[i][j - 1] + 1};
            int remove{dp[i - 1][j] + 1};
            int replace{dp[i - 1][j - 1] + (chars1[i - 1] != chars2[j - 1])};

            dp[i][j] = min({insert, remove, replace});
        }
    }

    int result = dp[m][n];
    
    // 将结果存入缓存
    editDistanceCache_.put(cacheKey, result);
    
    return result;
}

vector<string> KeywordRecommender::findCandidateWords(const string& query) {
    // 1.分字处理
    auto characters{textProcessor_.utf8Split(query)};

    // 2.收集所有相关行号
    unordered_set<int> unionLineNumbers;
    // 判断主要语言类型
    bool isPrimaryChinese{isPrimarilyChinese(query)};

    for (const auto& ch : characters) {
        auto lineNumbers{getLineNumbersFromIndex(ch)};
        unionLineNumbers.insert(lineNumbers.begin(), lineNumbers.end());
    }

    cout << "通过索引找到行号数量: " << unionLineNumbers.size() << endl;

    // 3.根据行号查找候选词
    unordered_set<string> candidateSet; // 使用set去重
    for (int lineNumber : unionLineNumbers) {
        // 根据行号和语言类型获取词汇
        string word{getWordFromDictionary(lineNumber, isPrimaryChinese)};
        if (!word.empty()) {
            candidateSet.insert(word);
        }
    }

    // 转换为vector返回
    return vector<string>(candidateSet.begin(), candidateSet.end());
}

vector<int>
KeywordRecommender::getLineNumbersFromIndex(const string& character) {
    if (textProcessor_.isChinese(character)) {
        // 查询中文索引
        auto* chineseIndex = dataManager_->getChineseIndexReader();
        return chineseIndex->getLineNumbers(character);
    } else if (textProcessor_.isEnglish(character)) {
        // 查询英文索引
        auto* englishIndex = dataManager_->getEnglishIndexReader();
        return englishIndex->getLineNumbers(character);
    }

    return {}; // 返回空vector
}

// 该函数在中英文混合输入时，会可能用英文词典行号去中文词典查词
// 不过最后判断编辑距离时，会过滤掉非中文词典的词汇
string KeywordRecommender::getWordFromDictionary(int lineNumber,
                                                 bool isChinese) {
    if (isChinese) {
        auto* chineseDict{dataManager_->getChineseDictionaryReader()};
        auto entries{chineseDict->getAllDictionaryEntries()};
        // 如果行号有效且在范围内
        if (lineNumber > 0 && lineNumber <= static_cast<int>(entries.size())) {
            return entries[lineNumber - 1].word; // 行号从1开始
        }
    } else {
        auto* englishDict{dataManager_->getEnglishDictionaryReader()};
        auto entries{englishDict->getAllDictionaryEntries()};
        if (lineNumber > 0 && lineNumber <= static_cast<int>(entries.size())) {
            return entries[lineNumber - 1].word; // 行号从1开始
        }
    }

    // lineNumber非法，返回空字符串
    return "";
}

int KeywordRecommender::getWordFrequency(const string& word) {
    // 先尝试中文词典
    auto* chineseDict = dataManager_->getChineseDictionaryReader();
    int frequency = chineseDict->getWordFrequency(word);

    if (frequency > 0) {
        return frequency;
    }

    // 再尝试英文词典
    auto* englishDict = dataManager_->getEnglishDictionaryReader();
    return englishDict->getWordFrequency(word);
}

vector<Candidate>
KeywordRecommender::selectTopK(const vector<Candidate>& candidates, int k) {
    // 使用自定义比较器排序
    vector<Candidate> sortedCandidates{candidates};
    std::sort(sortedCandidates.begin(), sortedCandidates.end(),
              CandidateComparator());

    // 返回前k个结果
    int count{min(k, static_cast<int>(sortedCandidates.size()))};
    return vector<Candidate>(sortedCandidates.begin(),
                             sortedCandidates.begin() + count);
}

bool KeywordRecommender::isPrimarilyChinese(const string& input) {
    auto characters{textProcessor_.utf8Split(input)};

    for (const auto& ch : characters) {
        if (textProcessor_.isChinese(ch)) {
            return true; // 只要有一个中文字符，就认为是中文输入
        }
    }

    return false; // 没发现任何中文
}