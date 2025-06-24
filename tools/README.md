# 搜索引擎数据生成工具

本目录包含两个用于生成搜索引擎数据文件的工具程序。

## 🔧 工具列表

### 1. build_dictionaries - 词典和索引生成工具

**功能**: 统一生成中英文词典库和索引库文件

**使用方法**:
```bash
./build_dictionaries [英文语料目录] [中文语料目录] [英文停用词文件] [中文停用词文件] [输出目录]
```

**默认参数**:
```bash
./build_dictionaries
# 等同于:
# ./build_dictionaries corpus/EN corpus/CN corpus/stopwords/en_stopwords.txt corpus/stopwords/cn_stopwords.txt data
```

**生成文件**:
- `data/dict_en.dat` - 英文词典文件
- `data/index_en.dat` - 英文索引文件  
- `data/dict_cn.dat` - 中文词典文件
- `data/index_cn.dat` - 中文索引文件

**特性**:
- ✅ 支持中英文混合语料处理
- ✅ 自动停用词过滤
- ✅ 统计信息报告
- ✅ 完整的错误处理

---

### 2. build_webpages - 网页库和倒排索引生成工具

**功能**: 生成网页库、偏移库和倒排索引库文件

**使用方法**:
```bash
./build_webpages [XML目录] [停用词文件] [输出目录] [保留网页数] [去重阈值]
```

**默认参数**:
```bash
./build_webpages
# 等同于:
# ./build_webpages corpus/webpages corpus/stopwords/cn_stopwords.txt data 10000 3
```

**生成文件**:
- `data/webpages.dat` - 网页库文件（存储网页内容）
- `data/offsets.dat` - 偏移库文件（记录网页在文件中的位置）
- `data/inverted_index.dat` - 倒排索引文件（词汇到文档的映射及TF-IDF权重）

**处理流程**:
1. 📖 **XML解析**: 解析RSS格式的XML文件，提取网页标题、链接、内容
2. 🔍 **去重处理**: 使用simhash算法去重，保留指定数量的唯一网页  
3. 📚 **构建网页库**: 生成网页库文件，同时创建偏移信息
4. 🔗 **倒排索引**: 中文分词，计算TF-IDF权重，构建倒排索引

**特性**:
- ✅ RSS/XML格式解析
- ✅ simhash相似度去重
- ✅ TF-IDF权重计算
- ✅ L2归一化处理
- ✅ 按Unicode排序

---

## 📊 使用示例

### 完整数据生成流程

```bash
# 1. 生成词典和索引库
./build_dictionaries

# 2. 生成网页库和倒排索引
./build_webpages

# 3. 查看生成的文件
ls -la data/
```

### 自定义参数示例

```bash
# 自定义词典生成
./build_dictionaries my_corpus/EN my_corpus/CN my_stopwords/en.txt my_stopwords/cn.txt output/

# 自定义网页库生成（保留5000个网页，相似度阈值为5）
./build_webpages my_xmls/ my_stopwords/cn.txt output/ 5000 5
```

---

## 📁 目录结构要求

```
project/
├── corpus/
│   ├── EN/                 # 英文语料文件
│   ├── CN/                 # 中文语料文件
│   ├── webpages/           # XML网页文件
│   └── stopwords/          # 停用词文件
│       ├── en_stopwords.txt
│       └── cn_stopwords.txt
├── data/                   # 输出目录
└── tools/                  # 工具程序
    ├── build_dictionaries
    └── build_webpages
```

---

## ⚠️ 注意事项

1. **内存需求**: 大语料处理可能需要较多内存
2. **磁盘空间**: 确保输出目录有足够空间
3. **文件格式**: XML文件需符合RSS格式规范
4. **权限**: 确保对输入和输出目录有读写权限

---

## 🐛 故障排除

**常见问题**:

- **"没有找到任何网页数据"**: 检查XML目录路径和文件格式
- **"处理失败"**: 检查停用词文件是否存在和可读
- **内存不足**: 减少处理的文件数量或增加系统内存

**日志输出**: 工具会输出详细的处理日志，便于调试和监控进度 