#pragma once

#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

// 目录操作工具类
class DirectoryUtils {
public:
    // 获取指定目录下所有指定扩展名的文件列表
    static std::vector<std::string> getFilesInDirectory(const std::string& dirPath, 
                                                       const std::string& extension = ".txt");
    
    // 检查给定路径是否为目录
    static bool isDirectory(const std::string& path);
    
    // 检查文件是否存在
    static bool fileExists(const std::string& file_path);

private:
    // 检查文件是否具有指定扩展名
    static bool hasExtension(const std::string& filename, const std::string& extension);
};

// 停用词管理类
class StopWordsManager {
public:
    explicit StopWordsManager(const std::string& stopWordsFile);

    // 判断是否是停用词
    bool isStopWord(const std::string& word) const;

    // 获取停用词数量
    size_t getStopWordsCount() const;

private:
    std::unordered_set<std::string> stopWords_; // 停用词集合

    void loadStopWords(const std::string& filePath); // 从文件加载停用词
};