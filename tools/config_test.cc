#include "../src/config/config.h"
#include <iostream>

using std::cout;
using std::endl;

int main() {
    cout << "=====================================" << endl;
    cout << "      简化配置管理类测试" << endl;
    cout << "=====================================" << endl;

    // 创建配置对象
    Configuration config("config/search_engine.conf");

    // 打印配置信息
    config.printConfig();

    cout << "\n=== 测试配置获取功能 ===" << endl;

    // 测试字符串配置
    cout << "服务器IP: " << config.getString("server_ip") << endl;
    cout << "数据目录: " << config.getString("data_dir") << endl;

    // 测试整数配置
    cout << "服务器端口: " << config.getInt("server_port") << endl;
    cout << "线程数: " << config.getInt("thread_num") << endl;
    cout << "最大编辑距离: " << config.getInt("max_edit_distance") << endl;

    cout << "\n=== 测试停用词功能 ===" << endl;

    // 测试停用词
    cout << "停用词总数: " << config.getStopWordList().size() << endl;
    cout << "测试停用词检查:" << endl;
    cout << "  'the' 是停用词: " << (config.isStopWord("the") ? "是" : "否")
         << endl;
    cout << "  'a' 是停用词: " << (config.isStopWord("a") ? "是" : "否")
         << endl;
    cout << "  'hello' 是停用词: " << (config.isStopWord("hello") ? "是" : "否")
         << endl;
    cout << "  '的' 是停用词: " << (config.isStopWord("的") ? "是" : "否")
         << endl;

    cout << "\n=== 测试默认值功能 ===" << endl;

    cout << "\n测试完成!" << endl;
    return 0;
}