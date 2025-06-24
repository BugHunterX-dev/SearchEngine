#pragma once

#include <string>
#include <map>
#include <set>
#include <fstream>
#include <iostream>

class Configuration {
public:
    // 构造函数，传入配置文件路径
    explicit Configuration(const std::string& filepath);

    // 获取配置内容的 map
    const std::map<std::string, std::string>& getConfigMap() const;

    // 获取停用词集合
    const std::set<std::string>& getStopWordList() const;

    // 获取配置值的便捷方法
    std::string getString(const std::string& key) const;
    int getInt(const std::string& key) const;

    // 停用词检查
    bool isStopWord(const std::string& word) const;

    // 打印配置信息
    void printConfig() const;

private:
    // 加载配置文件
    void loadConfigFile();
    
    // 加载停用词文件
    void loadStopWordsFile();
    
    // 初始化默认配置
    void initializeDefaults();

    // 辅助函数：去除字符串两端空白
    std::string trim(const std::string& str) const;

private:
    // 配置文件路径
    std::string filepath_;

    // 配置文件内容
    std::map<std::string, std::string> configMap_;

    // 停用词词集
    std::set<std::string> stopWordList_;
}; 