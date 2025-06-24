#include "../src/keywords/unified_tokenizer.h"
#include <iostream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

int main(int argc, char* argv[]) {
    cout << "=====================================" << endl;
    cout << "    中英文词典和索引库生成工具" << endl;
    cout << "=====================================" << endl;

    // 默认参数
    string enCorpusDir = "corpus/EN";
    string cnCorpusDir = "corpus/CN";
    string enStopwordsFile = "corpus/stopwords/en_stopwords.txt";
    string cnStopwordsFile = "corpus/stopwords/cn_stopwords.txt";
    string dataDir = "data";

    // 解析命令行参数
    if (argc > 1) {
        enCorpusDir = argv[1];
    }
    if (argc > 2) {
        cnCorpusDir = argv[2];
    }
    if (argc > 3) {
        enStopwordsFile = argv[3];
    }
    if (argc > 4) {
        cnStopwordsFile = argv[4];
    }
    if (argc > 5) {
        dataDir = argv[5];
    }

    cout << "配置信息:" << endl;
    cout << "  英文语料目录: " << enCorpusDir << endl;
    cout << "  中文语料目录: " << cnCorpusDir << endl;
    cout << "  英文停用词文件: " << enStopwordsFile << endl;
    cout << "  中文停用词文件: " << cnStopwordsFile << endl;
    cout << "  输出数据目录: " << dataDir << endl;
    cout << endl;

    try {
        // 创建统一分词器
        cout << "初始化统一分词器..." << endl;
        UnifiedTokenizer tokenizer(enStopwordsFile, cnStopwordsFile);

        // 生成输出文件路径
        string enDictFile = dataDir + "/dict_en.dat";
        string enIndexFile = dataDir + "/index_en.dat";
        string cnDictFile = dataDir + "/dict_cn.dat";
        string cnIndexFile = dataDir + "/index_cn.dat";

        cout << "将生成以下文件:" << endl;
        cout << "  英文词典: " << enDictFile << endl;
        cout << "  英文索引: " << enIndexFile << endl;
        cout << "  中文词典: " << cnDictFile << endl;
        cout << "  中文索引: " << cnIndexFile << endl;
        cout << endl;

        // 开始处理
        cout << "开始处理中英文语料..." << endl;
        bool success =
            tokenizer.processDirectories(enCorpusDir, cnCorpusDir, enDictFile,
                                         enIndexFile, cnDictFile, cnIndexFile);

        if (success) {
            cout << endl;
            cout << "词典和索引库生成成功！" << endl;
            cout << endl;

            // 打印统计信息
            tokenizer.printAllStatistics();

            cout << endl;
            cout << "生成的文件:" << endl;
            cout << "  " << enDictFile << " - 英文词典" << endl;
            cout << "  " << enIndexFile << " - 英文索引" << endl;
            cout << "  " << cnDictFile << " - 中文词典" << endl;
            cout << "  " << cnIndexFile << " - 中文索引" << endl;

        } else {
            cerr << "词典和索引库生成失败！" << endl;
            return 1;
        }

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
// ./build_dictionaries [英文语料目录] [中文语料目录] [英文停用词文件]
// [中文停用词文件] [输出目录]
//
// 示例：
// ./build_dictionaries corpus/EN corpus/CN corpus/stopwords/en_stopwords.txt
// corpus/stopwords/cn_stopwords.txt data
//
// 默认使用：
// ./build_dictionaries
//
// 将会在 data/ 目录下生成：
// - dict_en.dat     英文词典
// - index_en.dat    英文索引
// - dict_cn.dat     中文词典
// - index_cn.dat    中文索引