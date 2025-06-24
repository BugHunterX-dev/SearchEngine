#include "web_search_engine.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utf8cpp/utf8.h>

using std::cerr;
using std::cout;
using std::endl;
using std::make_unique;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

WebSearchEngine::WebSearchEngine(DataReaderManager* dataManager)
    : dataManager_{dataManager}
    , maxSummaryLength_{200}
    , jieba_{make_unique<cppjieba::Jieba>()}
    , stopWordsManager_{
          make_unique<StopWordsManager>("corpus/stopwords/cn_stopwords.txt")}
    , searchCache_(200) {}  // 默认缓存200个搜索结果

vector<SearchResult> WebSearchEngine::search(const string& query, int topN) {
    if (query.empty() || topN <= 0) {
        return {};
    }

    // 尝试从缓存获取结果
    SearchCacheKey cacheKey{query, topN};
    vector<SearchResult> cachedResult;
    if (searchCache_.get(cacheKey, cachedResult)) {
        cout << "从缓存获取搜索结果，查询词: " << query << ", 返回数量: " << topN << endl;
        return cachedResult;
    }

    cout << "开始搜索，查询词: " << query << "，返回数量: " << topN << endl;

    // 1.查询词标准化
    string normalizedQuery{normalizeQuery(query)};

    // 2.分词处理
    vector<string> terms{tokenizeQuery(normalizedQuery)};
    cout << "分词结果: ";
    for (const auto& term : terms) {
        cout << "[" << term << "] ";
    }
    cout << endl;

    // 3.执行搜索
    auto result = performSearch(terms, topN);
    
    // 将结果存入缓存
    if (!result.empty()) {
        searchCache_.put(cacheKey, result);
    }
    
    return result;
}

nlohmann::json WebSearchEngine::searchToJson(const string& query, int topN) {
    auto results{search(query, topN)};

    SearchResponse resp;
    resp.query = query;
    resp.results = results;
    resp.total = results.size();
    resp.timestamp = static_cast<int>(std::time(nullptr));

    return resp.toJson();
}

vector<SearchResult> WebSearchEngine::performSearch(const vector<string>& terms,
                                                 int topN) {
    if (terms.empty()) {
        return {};
    }

    // 获取网页库读取器
    auto* webPageReader{dataManager_->getWebPageLibraryReader()};
    if (!webPageReader) {
        cerr << "错误： 无法获取网页库读取器" << endl;
        return {};
    }

    cout << "=== 向量空间模型搜索 ===" << endl;
    cout << "查询词: ";
    for (const auto& term : terms) {
        cout << "[" << term << "] ";
    }
    cout << endl;

    // 1.使用交集策略查找包含所有查询词的文档
    vector<int> candidateDocs{findIntersectionDocuments(terms)};
    cout << "通过交集找到 " << candidateDocs.size() << " 个候选文档" << endl;

    if (candidateDocs.empty()) {
        cout << "没有找到包含所有查询词的文档" << endl;
        return {};
    }

    // 2.计算查询向量（基于TF-IDF）
    auto queryVector{calculateQueryVector(terms)};
    cout << "查询向量构建完成，维度: " << queryVector.size() << endl;

    // 3.计算每个候选文档与查询向量的余弦相似度
    vector<std::pair<int, double>> docScores;
    for (int docid : candidateDocs) {
        double similarity{calculateCosineSimilarity(terms, docid, queryVector)};
        if (similarity > 0) {
            docScores.emplace_back(docid, similarity);
        }
    }

    cout << "计算了 " << docScores.size() << " 个文档的余弦相似度" << endl;

    if (docScores.empty()) {
        return {};
    }

    // 4.对文档按余弦相似度排序
    std::sort(docScores.begin(), docScores.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) {
                      return a.second > b.second; // 按相似度降序
                  }
                  return a.first < b.first; // 相似度相同时按文档ID升序
              });

    // 5.构建搜索结果
    vector<SearchResult> results;
    int count{std::min(topN, static_cast<int>(docScores.size()))};

    for (int i = 0; i < count; ++i) {
        int docid{docScores[i].first};
        double similarity{docScores[i].second};

        // 根据文档id获取单个网页
        auto webPage{webPageReader->getWebPage(docid)};
        if (webPage) {
            string summary{generateSummary(webPage->content, terms)};
            string cleanTitle{cleanUtf8String(webPage->title)};
            string cleanLink{cleanUtf8String(webPage->link)};
            string cleanSummary{cleanUtf8String(summary)};
            
            results.emplace_back(docid, cleanTitle, cleanLink, cleanSummary,
                                 similarity);
        }
    }

    cout << "最终返回 " << results.size() << " 个搜索结果" << endl;
    return results;
}

// 实现交集策略：查找包含所有查询词的文档
vector<int>
WebSearchEngine::findIntersectionDocuments(const vector<string>& terms) {
    // 获取网页库读取器
    auto* webPageReader{dataManager_->getWebPageLibraryReader()};

    if (terms.empty()) {
        return {};
    }

    // 获取第一个词的文档列表作为初始集合
    unordered_set<int> resultSet;
    if (webPageReader->hasTerm(terms[0])) {
        // 获取第一个词的文档id和权重
        auto docs{webPageReader->getDocuments(terms[0])};
        for (const auto& [docid, weight] : docs) {
            resultSet.insert(docid);
        }
        cout << "词 [" << terms[0] << "] 出现在 " << resultSet.size()
             << " 个文档中" << endl;
    } else {
        cout << "词 [" << terms[0] << "] 未找到" << endl;
        return {}; // 如果一个词也没有找到，则返回空集合
    }

    // 对每个后续词汇求交集
    for (size_t i = 1; i < terms.size(); ++i) {
        if (!webPageReader->hasTerm(terms[i])) {
            cout << "词 [" << terms[i] << "] 未找到，交集为空" << endl;
            return {}; // 如果一个词未找到，则交集为空
        }

        unordered_set<int> currentSet;
        auto docs{webPageReader->getDocuments(terms[i])};
        for (const auto& [docid, weight] : docs) {
            currentSet.insert(docid);
        }
        cout << "词 [" << terms[i] << "] 出现在 " << currentSet.size()
             << " 个文档中" << endl;

        // 求交集
        unordered_set<int> intersection;
        for (int docid : resultSet) {
            // 检查resultSet中的每个文档是否在当前词的文档列表中
            if (currentSet.find(docid) != currentSet.end()) {
                // 既在resultSet中又在currentSet中，说明是交集
                intersection.insert(docid);
            }
        }
        resultSet = std::move(intersection);
        cout << "当前交集大小: " << resultSet.size() << endl;

        if (resultSet.empty()) {
            break; // 如果交集为空，提前退出
        }
    }

    return vector<int>(resultSet.begin(), resultSet.end());
}

// 计算查询向量的TF-IDF权重
unordered_map<string, double>
WebSearchEngine::calculateQueryVector(const vector<string>& queryTerms) {
    unordered_map<string, double> queryVector;
    auto* webPageReader{dataManager_->getWebPageLibraryReader()};
    size_t totalDocs{webPageReader->getWebPageCount()};

    // 1.计算查询中每个词的词频（TF）
    unordered_map<string, int> queryTF;
    for (const auto& term : queryTerms) {
        queryTF[term]++;
    }

    // 2.计算每个词的TF-IDF权重
    double normSquared{0.0}; // 累加向量中各分量的平方和
    for (const auto& [term, tf] : queryTF) {
        if (webPageReader->hasTerm(term)) {
            auto docs{webPageReader->getDocuments(term)};
            size_t df{docs.size()}; // 文档频率

            // 计算IDF: log2(总文档数 / (文档频率 + 1))
            double idf{std::log2(static_cast<double>(totalDocs) / (df + 1))};

            // 计算TF-IDF权重
            double tfidf{tf * idf};
            queryVector[term] = tfidf;
            normSquared += tfidf * tfidf;
        }
    }

    // 3.L2归一化
    if (normSquared > 0) {
        double norm{std::sqrt(normSquared)};
        for (auto& [term, weight] : queryVector) {
            weight /= norm;
        }
    }

    return queryVector;
}

// 获取文档向量（使用倒排索引中已归一化的权重）
unordered_map<string, double>
WebSearchEngine::getDocumentVector(int docid, const vector<string>& queryTerms) {
    unordered_map<string, double> docVector;
    auto* webPageReader{dataManager_->getWebPageLibraryReader()};

    for (const auto& term : queryTerms) {
        if (webPageReader->hasTerm(term)) {
            auto docs{webPageReader->getDocuments(term)};

            // 查找该文档对应该词的权重
            for (const auto& [id, weight] : docs) {
                if (id == docid) {
                    docVector[term] = weight; // 使用倒排索引中已归一化的权重
                    break;
                }
            }
        }
    }

    return docVector;
}

// 计算余弦相似度
double WebSearchEngine::calculateCosineSimilarity(
    const vector<string>& queryTerms, int docid,
    const unordered_map<string, double>& queryVector) {
    // 获取文档向量
    auto docVector{getDocumentVector(docid, queryTerms)};

    if (docVector.empty() || queryVector.empty()) {
        return 0.0;
    }

    // 计算点积
    double dotProduct{calculateDotProduct(queryVector, docVector)};

    // 由于查询向量和文档向量都已经归一化，余弦相似度就是点积
    return dotProduct;
}

// 计算两个向量的点积
double
WebSearchEngine::calculateDotProduct(const unordered_map<string, double>& vec1,
                                  const unordered_map<string, double>& vec2) {
    double dotProduct{0.0};

    // 注意：vec1 和 vec2 的 size() 通常是不相等的。
    // - vec1 可能是查询向量，包含所有查询词（在语料库中出现过的）
    // - vec2 可能是文档向量，仅包含当前文档中命中的查询词
    // 所以 vec2.size() ≤ vec1.size()，不应假设它们长度一致。
    // 遍历较小的向量以提高效率
    // unordered_map::find() 是 O(1) 平均复杂度，但遍历次数越少越快
    const auto& smaller{vec1.size() <= vec2.size() ? vec1 : vec2};
    const auto& larger{vec1.size() <= vec2.size() ? vec2 : vec1};

    for (const auto& [term, weight1] : smaller) {
        // 在较大向量中查找相同term
        auto it{larger.find(term)};
        if (it != larger.end()) {
            dotProduct += weight1 * it->second;
        }
    }

    return dotProduct;
}

string WebSearchEngine::generateSummary(const string& content,
                                     const vector<string>& terms) {
    if (content.empty()) {
        return "";
    }

    // 清理UTF-8字符，确保JSON序列化不会出错
    string cleanedContent{cleanUtf8String(content)};

    // 将正文转换为小写（只对英文有效）
    string lowerContent{cleanedContent};
    std::transform(lowerContent.begin(), lowerContent.end(),
                   lowerContent.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 寻找第一个匹配的关键词位置
    size_t bestPos{0};
    for (const auto& term : terms) {
        size_t pos{lowerContent.find(term)};
        if (pos != string::npos) {
            bestPos = pos;
            break;
        }
    }

    // 以找到的位置为中心，提取摘要
    size_t startPos{0};
    if (bestPos > static_cast<size_t>(maxSummaryLength_) / 2) {
        startPos = bestPos - static_cast<size_t>(maxSummaryLength_) / 2;
    }

    string summary{
        cleanedContent.substr(startPos, static_cast<size_t>(maxSummaryLength_))};

    // 如果从中间开始，添加省略号
    if (startPos > 0) {
        summary = "..." + summary;
    }
    // 如果关键词前面的内容足够长（大于摘要长度的一半），就从关键词前面适当位置开始截取，让关键词居中；
    // 否则从正文开头开始截取
    if (startPos + static_cast<size_t>(maxSummaryLength_) < cleanedContent.length()) {
        summary += "...";
    }

    // 高亮关键词
    return highlightKeywords(summary, terms);
}

string WebSearchEngine::normalizeQuery(const string& query) {
    string result{query};

    // 去除首尾空格
    size_t start{result.find_first_not_of(" \t\n\r")};
    if (start == string::npos) {
        return "";
    }
    size_t end{result.find_last_not_of(" \t\n\r")};
    result = result.substr(start, end - start + 1);

    // 压缩内部多个空格为单个空格
    string normalized;
    bool prevSpace{false};
    for (char c : result) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!prevSpace) {
                normalized += ' ';
                prevSpace = true;
            }
        } else {
            normalized += c;
            prevSpace = false;
        }
    }

    // 将整个字符串转换为小写（英文有效）
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return normalized;
}

string WebSearchEngine::highlightKeywords(const string& text,
                                       const vector<string>& terms) {
    string result{text};

    // 简单高亮：用【】包围关键词
    for (const auto& term : terms) {
        string lowerText{result};
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        size_t pos{0};
        while ((pos = lowerText.find(term, pos)) != string::npos) {
            // 在原始text中找到对应位置并高亮
            string original{result.substr(pos, term.length())};
            string highlighted = "【" + original + "】";
            result.replace(pos, term.length(), highlighted);

            // 更新位置为插入后的下一位
            pos += highlighted.length();

            // 重新生成lowerText，因为result已经变化
            lowerText = result;
            std::transform(lowerText.begin(), lowerText.end(),
                           lowerText.begin(),
                           [](unsigned char c) { return std::tolower(c); });
        }
    }

    return result;
}

vector<string> WebSearchEngine::tokenizeQuery(const string& query) {
    vector<string> terms;

    if (query.empty()) {
        return terms;
    }

    // 使用jieba进行中文分词
    vector<string> words;
    jieba_->Cut(query, words);

    // 过滤和处理分词结果
    for (const auto& word : words) {
        // 检查是否为有效的中文词或英文词
        bool isValid = false;

        // 检查中文字符
        auto it = word.begin();
        while (it != word.end()) {
            uint32_t codepoint = utf8::next(it, word.end());
            if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) { // 中文字符范围
                isValid = true;
                break;
            }
        }

        // 如果不是中文，检查是否为英文词
        if (!isValid && word.length() >= 2) {
            bool allAlpha = true;
            for (char c : word) {
                if (!std::isalpha(c)) {
                    allAlpha = false;
                    break;
                }
            }
            if (allAlpha) {
                isValid = true;
            }
        }

        // 添加有效词汇（排除停用词）
        if (isValid && !stopWordsManager_->isStopWord(word)) {
            terms.push_back(word);
        }
    }

    return terms;
}

string WebSearchEngine::cleanUtf8String(const string& input) {
    // 如果输入为空，直接返回空字符串
    if (input.empty()) {
        return "";
    }
    
    string result;
    result.reserve(input.size());
    
    // 第一步：尝试使用utf8cpp库进行安全的UTF-8处理
    try {
        auto it = input.begin();
        while (it != input.end()) {
            try {
                // 尝试解析下一个UTF-8字符
                uint32_t codepoint = utf8::next(it, input.end());
                
                // 检查是否为有效的字符范围
                if (codepoint == '\t' || codepoint == '\n' || codepoint == '\r' || codepoint == ' ' ||
                    (codepoint >= 0x20 && codepoint <= 0x7E) ||  // 基本ASCII可打印字符
                    (codepoint >= 0x4E00 && codepoint <= 0x9FFF) || // 中文汉字
                    (codepoint >= 0x3400 && codepoint <= 0x4DBF) || // 扩展A区汉字
                    (codepoint >= 0x3000 && codepoint <= 0x303F) || // 中文标点
                    (codepoint >= 0xFF00 && codepoint <= 0xFFEF) || // 全角字符
                    (codepoint >= 0xA0 && codepoint <= 0xFF)) {     // 拉丁补充字符
                    
                    // 将有效的Unicode代码点编码为UTF-8并添加到结果中
                    utf8::append(codepoint, std::back_inserter(result));
                } else {
                    // 无效字符，替换为空格
                    result += ' ';
                }
            } catch (const utf8::exception&) {
                // UTF-8解析错误，跳过这个字节，添加空格
                ++it;
                result += ' ';
            }
        }
        
        // 验证结果是否为有效的UTF-8
        if (!utf8::is_valid(result.begin(), result.end())) {
            throw std::runtime_error("结果仍然包含无效的UTF-8序列");
        }
        
        return result;
    } catch (...) {
        // 如果UTF-8处理失败，降级为严格的ASCII-only处理
        result.clear();
        result.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(input[i]);
            if ((c >= 0x20 && c <= 0x7E) || c == '\t' || c == '\n' || c == '\r' || c == ' ') {
                // 只保留ASCII可打印字符和常见空白字符
                result += c;
            } else {
                // 所有非ASCII字符替换为空格
                result += ' ';
            }
        }
        
        return result;
    }
}