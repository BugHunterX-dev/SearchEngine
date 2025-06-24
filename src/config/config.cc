#include "config.h"
#include <algorithm>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

Configuration::Configuration(const string& filepath)
    : filepath_(filepath) {
    // 初始化默认配置
    initializeDefaults();

    // 加载配置文件
    loadConfigFile();

    // 加载停用词文件
    loadStopWordsFile();
}

const std::map<string, string>& Configuration::getConfigMap() const {
    return configMap_;
}

const std::set<string>& Configuration::getStopWordList() const {
    return stopWordList_;
}

string Configuration::getString(const string& key) const {
    return configMap_.find(key)->second;
}

int Configuration::getInt(const string& key) const {
    return std::stoi(configMap_.find(key)->second);
}

bool Configuration::isStopWord(const string& word) const {
    return stopWordList_.find(word) != stopWordList_.end();
}

void Configuration::printConfig() const {
    cout << "=== 配置信息 ===" << endl;
    cout << "配置文件路径: " << filepath_ << endl;
    cout << "配置项数量: " << configMap_.size() << endl;
    cout << "停用词数量: " << stopWordList_.size() << endl;
    cout << endl;

    cout << "主要配置项:" << endl;
    for (const auto& pair : configMap_) {
        cout << "  " << pair.first << " = " << pair.second << endl;
    }

    cout << endl;
    cout << "停用词示例 (前10个):" << endl;
    int count = 0;
    for (const auto& word : stopWordList_) {
        if (count >= 10)
            break;
        cout << "  " << word << endl;
        ++count;
    }
    cout << "===============" << endl;
}

void Configuration::loadConfigFile() {
    std::ifstream file(filepath_);
    if (!file.is_open()) {
        cout << "配置文件不存在，使用默认配置: " << filepath_ << endl;
        return;
    }

    string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        // 去除首尾空白
        line = trim(line);

        // 跳过空行和注释行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 解析键值对
        size_t pos = line.find('=');
        if (pos == string::npos) {
            cerr << "配置文件第 " << lineNumber << " 行格式错误: " << line
                 << endl;
            continue;
        }

        string key = trim(line.substr(0, pos));
        string value = trim(line.substr(pos + 1));

        if (key.empty()) {
            cerr << "配置文件第 " << lineNumber << " 行键名为空: " << line
                 << endl;
            continue;
        }

        configMap_[key] = value;
    }

    file.close();
    cout << "配置文件加载成功: " << filepath_ << "，共加载 "
         << configMap_.size() << " 个配置项" << endl;
}

void Configuration::loadStopWordsFile() {
    // 从配置中获取停用词文件路径
    string enStopwordsFile = getString("en_stopwords_file");
    string cnStopwordsFile = getString("cn_stopwords_file");

    // 加载英文停用词
    std::ifstream enFile(enStopwordsFile);
    if (enFile.is_open()) {
        string word;
        int count = 0;
        while (std::getline(enFile, word)) {
            word = trim(word);
            if (!word.empty()) {
                stopWordList_.insert(word);
                ++count;
            }
        }
        enFile.close();
        cout << "加载英文停用词成功: " << enStopwordsFile << "，共 " << count
             << " 个词" << endl;
    } else {
        cout << "英文停用词文件不存在: " << enStopwordsFile << endl;
    }

    // 加载中文停用词
    std::ifstream cnFile(cnStopwordsFile);
    if (cnFile.is_open()) {
        string word;
        int count = 0;
        while (std::getline(cnFile, word)) {
            word = trim(word);
            if (!word.empty()) {
                stopWordList_.insert(word);
                ++count;
            }
        }
        cnFile.close();
        cout << "加载中文停用词成功: " << cnStopwordsFile << "，共 " << count
             << " 个词" << endl;
    } else {
        cout << "中文停用词文件不存在: " << cnStopwordsFile << endl;
    }
}

void Configuration::initializeDefaults() {
    // 服务器配置
    configMap_["server_ip"] = "0.0.0.0";
    configMap_["server_port"] = "8080";
    configMap_["thread_num"] = "4";
    configMap_["queue_size"] = "100";

    // 路径配置
    configMap_["data_dir"] = "data";
    configMap_["corpus_dir"] = "corpus";
    configMap_["en_corpus_dir"] = "corpus/EN";
    configMap_["cn_corpus_dir"] = "corpus/CN";
    configMap_["webpages_dir"] = "corpus/webpages";
    configMap_["en_stopwords_file"] = "corpus/stopwords/en_stopwords.txt";
    configMap_["cn_stopwords_file"] = "corpus/stopwords/cn_stopwords.txt";

    // 数据文件路径
    configMap_["dict_en_file"] = "data/dict_en.dat";
    configMap_["index_en_file"] = "data/index_en.dat";
    configMap_["dict_cn_file"] = "data/dict_cn.dat";
    configMap_["index_cn_file"] = "data/index_cn.dat";
    configMap_["webpages_file"] = "data/webpages.dat";
    configMap_["offsets_file"] = "data/offsets.dat";
    configMap_["inverted_index_file"] = "data/inverted_index.dat";

    // 算法参数
    configMap_["max_edit_distance"] = "2";
    configMap_["default_recommend_k"] = "10";
    configMap_["default_search_top_n"] = "5";
    configMap_["max_summary_length"] = "200";
    configMap_["simhash_top_k"] = "10000";
    configMap_["simhash_threshold"] = "3";

    // 缓存配置
    configMap_["recommend_cache_size"] = "500";
    configMap_["edit_distance_cache_size"] = "2000";
    configMap_["search_cache_size"] = "200";
}

string Configuration::trim(const string& str) const {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos) {
        return "";
    }

    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}