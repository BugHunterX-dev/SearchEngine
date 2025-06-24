#include "../src/webpages/page_processor.h"
#include <iostream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

int main(int argc, char* argv[]) {
    cout << "=====================================" << endl;
    cout << "   网页库和倒排索引生成工具" << endl;
    cout << "=====================================" << endl;

    // 默认参数
    string xmlDir = "corpus/webpages";
    string stopwordsFile = "corpus/stopwords/cn_stopwords.txt";
    string dataDir = "data";
    int topK = 10000;  // 去重后保留的网页数量
    int threshold = 3; // simhash去重阈值

    // 解析命令行参数
    if (argc > 1) {
        xmlDir = argv[1];
    }
    if (argc > 2) {
        stopwordsFile = argv[2];
    }
    if (argc > 3) {
        dataDir = argv[3];
    }
    if (argc > 4) {
        topK = std::stoi(argv[4]);
    }
    if (argc > 5) {
        threshold = std::stoi(argv[5]);
    }

    cout << "配置信息:" << endl;
    cout << "  XML网页目录: " << xmlDir << endl;
    cout << "  停用词文件: " << stopwordsFile << endl;
    cout << "  输出数据目录: " << dataDir << endl;
    cout << "  去重保留数量: " << topK << endl;
    cout << "  去重相似度阈值: " << threshold << endl;
    cout << endl;

    try {
        // 创建页面处理器
        cout << "初始化网页处理器..." << endl;
        PageProcessor processor(stopwordsFile);

        // 生成输出文件路径
        string webpagesFile = dataDir + "/webpages.dat";
        string offsetsFile = dataDir + "/offsets.dat";
        string invertedIndexFile = dataDir + "/inverted_index.dat";

        cout << "将生成以下文件:" << endl;
        cout << "  网页库: " << webpagesFile << endl;
        cout << "  偏移库: " << offsetsFile << endl;
        cout << "  倒排索引: " << invertedIndexFile << endl;
        cout << endl;

        // 第一步：解析XML文件
        cout << "步骤1: 解析XML网页文件..." << endl;
        auto webpages = processor.parseAllXmlFiles(xmlDir);

        if (webpages.empty()) {
            cerr << "没有找到任何网页数据！" << endl;
            return 1;
        }

        cout << "成功解析 " << webpages.size() << " 个网页" << endl;

        // 第二步：网页去重
        cout << endl;
        cout << "步骤2: 使用simhash进行网页去重..." << endl;
        processor.deduplicateWebPages(webpages, topK, threshold);
        cout << "去重完成" << endl;

        // 第三步：构建网页库和偏移库
        cout << endl;
        cout << "步骤3: 构建网页库和偏移库..." << endl;
        bool webpagesSuccess =
            processor.buildWebPagesAndOffsets(webpagesFile, offsetsFile);

        if (!webpagesSuccess) {
            cerr << "网页库和偏移库构建失败！" << endl;
            return 1;
        }

        cout << "网页库和偏移库构建成功" << endl;

        // 第四步：构建倒排索引
        cout << endl;
        cout << "步骤4: 构建倒排索引库..." << endl;
        bool indexSuccess = processor.buildInvertedIndex(invertedIndexFile);

        if (!indexSuccess) {
            cerr << "倒排索引库构建失败！" << endl;
            return 1;
        }

        cout << "倒排索引库构建成功" << endl;

        // 完成
        cout << endl;
        cout << "生成的文件:" << endl;
        cout << "  " << webpagesFile << " - 网页库（包含所有网页内容）" << endl;
        cout << "  " << offsetsFile << " - 偏移库（网页在文件中的位置信息）"
             << endl;
        cout << "  " << invertedIndexFile
             << " - 倒排索引（词汇 -> 文档ID和权重）" << endl;

    } catch (const std::exception& e) {
        cerr << "发生异常: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "发生未知异常！" << endl;
        return 1;
    }

    cout << endl;
    cout << "=====================================" << endl;
    cout << "           生成完成！" << endl;
    cout << "=====================================" << endl;

    return 0;
}

// 使用说明：
// ./build_webpages [XML目录] [停用词文件] [输出目录] [保留网页数] [去重阈值]
//
// 示例：
// ./build_webpages corpus/webpages corpus/stopwords/cn_stopwords.txt data 10000
// 3
//
// 默认使用：
// ./build_webpages
//
// 将会在 data/ 目录下生成：
// - webpages.dat        网页库（存储所有网页内容）
// - offsets.dat         偏移库（记录每个网页在文件中的位置）
// - inverted_index.dat  倒排索引（词汇到文档的映射及TF-IDF权重）
//
// 处理流程：
// 1. 解析XML文件，提取网页标题、链接、内容
// 2. 使用simhash算法去重，保留指定数量的唯一网页
// 3. 构建网页库文件，同时生成偏移信息
// 4. 对网页内容进行中文分词，计算TF-IDF权重，构建倒排索引