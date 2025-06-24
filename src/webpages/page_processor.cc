#include "page_processor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <tinyxml2.h>
#include <unordered_map>
#include <unordered_set>
#include <utf8cpp/utf8.h>

using namespace tinyxml2;
using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::regex;
using std::regex_replace;
using std::string;
using std::vector;
using namespace simhash;
using std::make_unique;
using std::ofstream;
using std::unordered_map;
using std::unordered_set;

// 构造函数实现
PageProcessor::PageProcessor(const string& stopwordsFile)
    : simhasher_{make_unique<simhash::Simhasher>()}
    , stopWordsManager_{make_unique<StopWordsManager>(stopwordsFile)}
    , jieba_{make_unique<cppjieba::Jieba>()} {}

void PageProcessor::deduplicateWebPages(const vector<WebPage>& webpages,
                                        int topk, int threshold) {
    vector<uint64_t> uniqueHashes;

    cout << "开始进行网页去重，共" << webpages.size() << "个网页..." << endl;

    for (const auto& webpage : webpages) {
        uint64_t hashValue = 0;
        simhasher_->make(webpage.content, topk, hashValue);

        bool isDup = false;
        for (const auto& hash : uniqueHashes) {
            if (simhasher_->isEqual(hashValue, hash, threshold)) {
                isDup = true;
                break;
            }
        }

        if (!isDup) {
            uniqueHashes.push_back(hashValue);
            uniquePages_.push_back(webpage);
        }
    }

    cout << "去重完成，去重前" << webpages.size() << "个网页，去重后"
         << uniquePages_.size() << "个网页" << endl;
}

vector<WebPage> PageProcessor::parseXmlFile(const string& xmlPath) {
    vector<WebPage> webpages;

    XMLDocument doc;
    if (doc.LoadFile(xmlPath.c_str()) != XML_SUCCESS) {
        cerr << "Failed to load XML file: " << xmlPath << endl;
        return webpages;
    }

    // RSS格式解析
    XMLElement* rss{doc.FirstChildElement("rss")};
    if (rss) {
        XMLElement* channel{rss->FirstChildElement("channel")};
        if (channel) {
            parseRssFormat(channel, webpages);
        }
    }

    cout << "从文件 " << xmlPath << " 解析出 " << webpages.size() << " 个网页"
         << endl;
    return webpages;
}

vector<WebPage> PageProcessor::parseAllXmlFiles(const string& xmlDirectory) {
    vector<WebPage> allWebpages;

    // 目录流操作
    DIR* dir = opendir(xmlDirectory.c_str());
    if (!dir) {
        cerr << "无法打开目录：" << xmlDirectory << endl;
        return allWebpages;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 "." 和 ".." 目录
        string filename{entry->d_name};
        if (filename == "." || filename == "..") {
            continue;
        }

        // 构建完整文件路径
        string fullPath{xmlDirectory + "/" + filename};

        // 检查是否为普通文件
        if (entry->d_type == DT_REG) {
            // 检查扩展名是否为 .xml
            if (filename.length() >= 4 &&
                filename.substr(filename.length() - 4) == ".xml") {
                auto webpages{parseXmlFile(fullPath)};
                allWebpages.insert(allWebpages.end(), webpages.begin(),
                                   webpages.end());
            }
        }
    }

    closedir(dir);

    cout << "总共解析出 " << allWebpages.size() << " 个网页" << endl;
    return allWebpages;
}

void PageProcessor::parseRssFormat(XMLElement* channel,
                                   vector<WebPage>& webpages) {
    XMLElement* item{channel->FirstChildElement("item")};
    while (item) {
        WebPage webpage;

        // 提取title
        XMLElement* titleElem{item->FirstChildElement("title")};
        if (titleElem && titleElem->GetText()) {
            webpage.title = cleanText(titleElem->GetText());
        }

        // 提取link
        XMLElement* linkElem{item->FirstChildElement("link")};
        if (linkElem && linkElem->GetText()) {
            webpage.link = trim(linkElem->GetText());
        }

        // 提取content，优先使用content:encoded，其次content，最后description
        XMLElement* contentElem{item->FirstChildElement("content:encoded")};
        if (contentElem && contentElem->GetText()) {
            webpage.content = cleanText(contentElem->GetText());
        } else {
            contentElem = item->FirstChildElement("content");
            if (contentElem && contentElem->GetText()) {
                webpage.content = cleanText(contentElem->GetText());
            } else {
                XMLElement* descElem{item->FirstChildElement("description")};
                if (descElem && descElem->GetText()) {
                    webpage.content = cleanText(descElem->GetText());
                }
            }
        }

        // 只有当有content或description才分配docid并添加
        if (!webpage.content.empty()) {
            webpage.docid = nextDocId_++;
            webpages.push_back(webpage);
        }

        item = item->NextSiblingElement("item");
    }
}

string PageProcessor::cleanText(const string& rawText) {
    if (rawText.empty()) {
        return "";
    }

    string result{removeCDATA(rawText)};
    result = removeHtmlTags(result);
    result = trim(result);

    return result;
}

string PageProcessor::removeCDATA(const string& text) {
    string result{text};

    // 去除 <![CDATA[ 和 ]]>
    // 寻找子串首次出现的位置
    size_t start{result.find("<![CDATA[")};
    // start == string::npos 代表查找失败
    if (start != string::npos) {
        result.erase(start, 9); // 删除 "<![CDATA["

        size_t end{result.find("]]>")};
        if (end != string::npos) {
            result.erase(end, 3); // 删除 "]]>"
        }
    }

    return result;
}

string PageProcessor::removeHtmlTags(const string& text) {
    // 使用正则表达式去除HTML标签
    regex htmlTag{"<[^>]*>"};
    string result{regex_replace(text, htmlTag, " ")};

    // 去除多余的空白字符
    regex multiSpace("\\s+");
    result = regex_replace(result, multiSpace, " ");

    return result;
}

string PageProcessor::trim(const string& text) {
    const string whitespce{" \t\n\r\f\v"};

    size_t start{text.find_first_not_of(whitespce)};
    if (start == string::npos) {
        return "";
    }

    size_t end{text.find_last_not_of(whitespce)};

    return text.substr(start, end - start + 1);
}

bool PageProcessor::buildWebPagesAndOffsets(const string& outputPath,
                                            const string& offsetPath) {
    // 打开网页库输出文件
    ofstream ofsPage{outputPath, ios::out | ios::trunc};
    if (!ofsPage) {
        cerr << "保存网页库失败：" << outputPath << endl;
        return false;
    }

    // 打开偏移库输出文件
    ofstream ofsOffset{offsetPath, ios::out | ios::trunc};
    if (!ofsOffset) {
        cerr << "保存偏移库失败：" << offsetPath << endl;
        return false;
    }

    // 遍历所有已去重的网页
    for (const auto& page : uniquePages_) {
        // 记录当前网页在网页库中的起始偏移（字节数）
        size_t offset{static_cast<size_t>(ofsPage.tellp())};

        // 写入网页内容到网页库文件
        ofsPage << "<doc>\n"
                << "  <docid>" << page.docid << "</docid>\n"
                << "  <link>" << page.link << "</link>\n"
                << "  <title>" << page.title << "</title>\n"
                << "  <content>" << page.content << "</content>\n"
                << "</doc>\n";

        // 计算本网页内容长度（字节数）
        size_t length{static_cast<size_t>(ofsPage.tellp()) - offset};

        // 写入当前网页的偏移信息到偏移库文件
        // 格式：docid offset length
        ofsOffset << page.docid << " " << offset << " " << length << "\n";
    }

    cout << "成功保存 " << uniquePages_.size() << " 个网页到文件："
         << outputPath << endl;
    cout << "成功建立网页偏移库到文件：" << offsetPath << endl;
    return true;
}

bool PageProcessor::buildInvertedIndex(const string& outputPath) {
    if (uniquePages_.empty()) {
        cerr << "没有去重后的网页数据，请先调用deduplicateWebPages" << endl;
        return false;
    }

    cout << "开始建立倒排索引库，共处理 " << uniquePages_.size() << " 个网页..."
         << endl;

    // 数据结构声明

    // 词频，外层unordered_map的key是词，内层unordered_map的key是文档id，value是词频
    unordered_map<string, unordered_map<int, int>> termFrequency;
    // 文档频率，某个词在多少个文档中出现过，key是词，value是文档数
    unordered_map<string, int> documentFrequency;
    // 文档总词数，key是文档id，value是文档总词数
    unordered_map<int, int> docTotalWords;
    // TF-IDF权重，外层unordered_map的key是词，内层unordered_map的key是文档id，value是TF-IDF权重
    unordered_map<string, unordered_map<int, double>> tfidfWeights;

    // 1.分词和停用词过滤
    tokenizationAndFiltering(termFrequency, documentFrequency, docTotalWords);

    // 2.计算TF-IDF权重
    calculateTfIdfWeights(termFrequency, documentFrequency, tfidfWeights);

    // 3.L2归一化
    performL2Normalization(tfidfWeights);

    // 4.生成倒排索引文件
    return generateInvertedIndexFile(tfidfWeights, outputPath);
}

void PageProcessor::tokenizationAndFiltering(
    unordered_map<string, unordered_map<int, int>>& termFrequency,
    unordered_map<string, int>& documentFrequency,
    unordered_map<int, int>& docTotalWords) {
    cout << "正在进行分词和停用词过滤..." << endl;

    // 循环处理每一个文档
    for (const auto& page : uniquePages_) {
        cout << "处理文档 " << page.docid << "..." << endl;

        // 使用jieba进行分词
        vector<string> words;
        jieba_->Cut(page.content, words);

        int validWords{0};
        unordered_set<string> uniqueWordsInDoc; // 用于统计文档频率

        // 处理当前文档中的每一个词
        for (const auto& word : words) {
            // 检查是否为中文有效词
            bool hasChineseChar{false};
            auto it{word.begin()};
            while (it != word.end()) {
                uint32_t codepoint{utf8::next(it, word.end())};
                if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
                    hasChineseChar = true;
                    break;
                }
            }
            if (!hasChineseChar) {
                continue;
            }

            // 检查是否为停用词
            if (stopWordsManager_->isStopWord(word)) {
                continue;
            }

            // 统计词频
            termFrequency[word][page.docid]++;
            validWords++;

            // 统计文档频率（每个文档中每个词只计算一次）
            if (uniqueWordsInDoc.find(word) == uniqueWordsInDoc.end()) {
                // 此时说明该词在当前文档中是第一次出现，需要统计文档频率
                uniqueWordsInDoc.insert(word);
                // 包含该词的文档数+1
                documentFrequency[word]++;
            }
        }

        // 统计文档总词数
        docTotalWords[page.docid] = validWords;
        cout << "文档 " << page.docid << " 处理完成，有效词数：" << validWords
             << endl;
    }

    cout << "分词完成！总共提取了 " << termFrequency.size() << " 个不同的词"
         << endl;
}

void PageProcessor::calculateTfIdfWeights(
    const unordered_map<string, unordered_map<int, int>>& termFrequency,
    const unordered_map<string, int>& documentFrequency,
    unordered_map<string, unordered_map<int, double>>& tfidfWeights) {
    cout << "正在计算TF-IDF权重..." << endl;

    int totalDocuments{static_cast<int>(uniquePages_.size())};

    // 处理每一个词
    for (const auto& [term, docFreqMap] : termFrequency) {
        // 计算IDF = log2(总文档数 / (包含该词的文档数 + 1))
        // 包含该词的文档数+1是为了避免分母为0
        int df{documentFrequency.at(term)};
        double idf{std::log2(static_cast<double>(totalDocuments) / (df + 1))};

        // 处理当前词在每一个文档中的TF-IDF权重
        // tf是这个词在当前文档出现的次数
        for (const auto& [docid, tf] : docFreqMap) {
            // 计算TF-IDF权重
            double tfidf{tf * idf};
            tfidfWeights[term][docid] = tfidf;
        }
    }

    cout << "TF-IDF计算完成！" << endl;
}

void PageProcessor::performL2Normalization(
    unordered_map<string, unordered_map<int, double>>& tfidfWeights) {
    cout << "正在进行L2归一化..." << endl;

    // 计算每个文档的权重向量的L2范数，即sqrt(w1^2 + w2^2 +...+ wn^2)
    unordered_map<int, double> docL2Norms;

    // 处理每一个文档
    for (const auto& page : uniquePages_) {
        double normSquared{0.0};

        // 计算当前文档所有词的TF-IDF权重的平方和
        for (const auto& [term, docWeights] : tfidfWeights) {
            const auto& it{docWeights.find(page.docid)};
            if (it != docWeights.end()) {
                double weight{it->second};
                normSquared += weight * weight;
            }
        }

        // 计算L2范数
        docL2Norms[page.docid] = sqrt(normSquared);
    }

    // 对TF-IDF权重进行L2归一化
    for (auto& [term, docWeights] : tfidfWeights) {
        for (auto& [docid, weight] : docWeights) {
            if (docL2Norms[docid] > 0) {
                weight /= docL2Norms[docid]; // L2归一化
            }
        }
    }

    cout << "L2归一化完成！" << endl;
}

bool PageProcessor::generateInvertedIndexFile(
    const unordered_map<string, unordered_map<int, double>>& tfidfWeights,
    const string& outputPath) {
    cout << "正在生成倒排索引文件：" << outputPath << endl;

    // 打开或创建倒排索引文件
    ofstream outFile{outputPath, ios::out | ios::trunc};
    if (!outFile) {
        cerr << "保存倒排索引文件失败：" << outputPath << endl;
        return false;
    }

    // 设置输出精度
    outFile << std::fixed << std::setprecision(6);

    // 将词汇按Unicode排序
    cout << "正在对词汇进行Unicode排序..." << endl;
    vector<string> sortedTerms;
    sortedTerms.reserve(tfidfWeights.size());
    for (const auto& [term, docWeights] : tfidfWeights) {
        sortedTerms.push_back(term);
    }

    // 对词汇进行Unicode排序
    std::sort(sortedTerms.begin(), sortedTerms.end());

    cout << "词汇排序完成，开始写入文件..." << endl;

    // 写入倒排索引文件
    int processedTerms{0};
    for (const string& term : sortedTerms) {
        const auto& docWeights{tfidfWeights.at(term)};

        // 按照格式输出：词 文档id1 权重1 文档id2 权重2...
        outFile << term;

        // 按文档ID排序
        vector<pair<int, double>> sortedDocs(docWeights.begin(),
                                             docWeights.end());
        std::sort(sortedDocs.begin(), sortedDocs.end());

        // 遍历每一个文档
        for (const auto& [docid, weight] : sortedDocs) {
            // 只输出权重不为0的文档
            if (weight > 0) {
                outFile << " " << docid << " " << weight;
            }
        }

        outFile << "\n";

        processedTerms++;
        if (processedTerms % 1000 == 0) {
            cout << "已处理 " << processedTerms << " 个词..." << endl;
        }
    }

    outFile.close();

    cout << "倒排索引库生成完成！" << endl;
    cout << "统计信息：" << endl;
    cout << "- 总词汇数：" << tfidfWeights.size() << endl;
    cout << "- 总文档数：" << uniquePages_.size() << endl;
    cout << "- 输出文件：" << outputPath << endl;

    return true;
}
