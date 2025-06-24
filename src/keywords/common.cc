#include "common.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;

// ============ DirectoryUtils类实现 ===============

// 获取指定目录下所有指定扩展名的文件列表
vector<string> DirectoryUtils::getFilesInDirectory(const string& dirPath,
                                             const string& extension) {
    vector<string> files;

    // 打开目录
    DIR* dir{opendir(dirPath.c_str())};
    if (dir == nullptr) {
        cerr << "[DirectoryUtils] Failed to open directory " << dirPath << ": "
             << strerror(errno) << endl;
        return files;
    }

    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string filename{entry->d_name};

        // 跳过当前目录和父目录
        if (filename == "." || filename == "..") {
            continue;
        }

        // 检查完整文件路径
        string fullPath{dirPath + "/" + filename};

        // 检查是否为普通文件且具有正确扩展名
        if (entry->d_type == DT_REG && hasExtension(filename, extension)) {
            files.push_back(fullPath);
        }
    }

    // 关闭目录
    closedir(dir);

    return files;
}

// 检查给定路径是否为目录
bool DirectoryUtils::isDirectory(const string& path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0) {
        return false;
    }
    return S_ISDIR(pathStat.st_mode);
}

// 检查文件是否存在
bool DirectoryUtils::fileExists(const string& file_path) {
    struct stat buffer;
    return (stat(file_path.c_str(), &buffer) == 0);
}

// 检查文件是否具有指定扩展名
bool DirectoryUtils::hasExtension(const string& filename,
                                  const string& extension) {
    if (filename.length() < extension.length()) {
        return false;
    }
    return filename.substr(filename.length() - extension.length()) == extension;
}

// ============ StopWordsManager类实现 ===============

StopWordsManager::StopWordsManager(const string& stopWordsFile) {
    loadStopWords(stopWordsFile);
}

// 从文件加载停用词
void StopWordsManager::loadStopWords(const string& filePath) {
    ifstream ifs{filePath};
    if (!ifs.is_open()) {
        cerr << "[StopWordsManager] Failed to open stopwords file: " << filePath
             << endl;
        return;
    }

    string word;
    while (std::getline(ifs, word)) {
        // 移除行尾的空白字符
        word.erase(word.find_last_not_of(" \t\n\r") + 1);
        if (!word.empty()) {
            stopWords_.insert(word);
        }
    }

    cout << "[StopWordsManager] Loaded " << stopWords_.size()
         << " stopwords from " << filePath << endl;
}

// 判断是否是停用词
bool StopWordsManager::isStopWord(const string& word) const {
    return stopWords_.find(word) != stopWords_.end();
}

// 获取停用词数量
size_t StopWordsManager::getStopWordsCount() const {
    return stopWords_.size();
}