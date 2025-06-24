# C++搜索引擎项目完整类图

## 最详细的Mermaid类图代码

```mermaid
classDiagram
    %% ===== 服务器核心架构层 =====
    class SearchEngineServer {
        -ThreadPool threadPool_
        -TcpServer tcpServer_
        -unique_ptr~DataReaderManager~ dataManager_
        -string dataDirectory_
        -bool running_
        +SearchEngineServer(size_t threadNum, size_t queueSize, string ip, unsigned short port)
        +~SearchEngineServer()
        +start() void
        +stop() void
        +setDataDirectory(string dataDir) void
        -onNewConnection(TcpConnectionPtr connection) void
        -onMessage(TcpConnectionPtr connection) void
        -onClose(TcpConnectionPtr connection) void
    }

    class TcpServer {
        -Acceptor acceptor_
        -EventLoop loop_
        +TcpServer(string ip, unsigned short port)
        +~TcpServer()
        +start() void
        +stop() void
        +setAllCallback(TcpConnectionCallback cb1, TcpConnectionCallback cb2, TcpConnectionCallback cb3) void
    }

    class Acceptor {
        -Socket sock_
        -InetAddress addr_
        +Acceptor(string ip, unsigned short port)
        +~Acceptor()
        +ready() void
        +accept() int
        +fd() int
        -setReuseAddr() void
        -setReusePort() void
        -bind() void
        -listen() void
    }

    class EventLoop {
        +loop() void
        +unloop() void
        +wakeup() void
        +updateChannel() void
    }

    class TcpConnection {
        +send() void
        +shutdown() void
        +setCallbacks() void
    }

    class ThreadPool {
        -size_t threadNum_
        -vector~thread~ threads_
        -size_t queSize_
        -TaskQueue taskQueue_
        -bool isExit_
        +ThreadPool(size_t threadNum, size_t queSize)
        +~ThreadPool()
        +start() void
        +stop() void
        +addTask(Task task) void
        -getTask() Task
        -doTask() void
    }

    class TaskQueue {
        -queue~Task~ taskQueue_
        -mutex mutex_
        -condition_variable condition_
        +push(Task task) void
        +pop() Task
        +size() size_t
        +empty() bool
    }

    class Socket {
        -int fd_
        +Socket()
        +Socket(int fd)
        +~Socket()
        +fd() int
        +shutDownWrite() void
    }

    class InetAddress {
        -sockaddr_in addr_
        +InetAddress(string ip, unsigned short port)
        +InetAddress(sockaddr_in addr)
        +~InetAddress()
        +ip() string
        +port() unsigned short
        +getInetAddrPtr() const sockaddr_in*
    }

    class NonCopyable {
        <<abstract>>
        #NonCopyable()
        #~NonCopyable()
        +NonCopyable(const NonCopyable&) = delete
        +operator=(const NonCopyable&) = delete
    }

    %% ===== 业务逻辑层 =====
    class SearchEngineTask {
        -string message_
        -TcpConnectionPtr connection_
        -DataReaderManager* dataManager_
        -static unique_ptr~WebSearchEngine~ webSearchEngine_
        -static unique_ptr~KeywordRecommender~ keywordRecommender_
        -static bool initialized_
        +SearchEngineTask(string message, TcpConnectionPtr connection, DataReaderManager* dataManager)
        +~SearchEngineTask()
        +process() void
        -handleKeywordRecommendRequest(TLVMessage request) void
        -handleSearchRequest(TLVMessage request) void
        -sendResponse(TLVMessage response) void
        -sendErrorResponse(string errorMessage, int errorCode) void
        -initializeBusinessComponents() void
    }

    %% ===== 数据访问层 =====
    class DataReaderManager {
        -unique_ptr~ChineseDictionaryReader~ chineseDictReader_
        -unique_ptr~EnglishDictionaryReader~ englishDictReader_
        -unique_ptr~ChineseIndexReader~ chineseIndexReader_
        -unique_ptr~EnglishIndexReader~ englishIndexReader_
        -unique_ptr~WebPageLibraryReader~ webPageLibraryReader_
        +DataReaderManager()
        +~DataReaderManager()
        +initialize(string dataDir) bool
        +getChineseDictionaryReader() ChineseDictionaryReader*
        +getEnglishDictionaryReader() EnglishDictionaryReader*
        +getChineseIndexReader() ChineseIndexReader*
        +getEnglishIndexReader() EnglishIndexReader*
        +getWebPageLibraryReader() WebPageLibraryReader*
        +setChineseDictionaryReader(unique_ptr~ChineseDictionaryReader~ reader) void
        +setEnglishDictionaryReader(unique_ptr~EnglishDictionaryReader~ reader) void
        +setChineseIndexReader(unique_ptr~ChineseIndexReader~ reader) void
        +setEnglishIndexReader(unique_ptr~EnglishIndexReader~ reader) void
        +setWebPageLibraryReader(unique_ptr~WebPageLibraryReader~ reader) void
    }

    class DictionaryReader {
        <<abstract>>
        +~DictionaryReader()
        +loadDictionary(string dictPath) bool
        +getWordFrequency(string word) int
        +getAllDictionaryEntries() vector~DictionaryEntry~
        +getDictionarySize() size_t
    }

    class ChineseDictionaryReader {
        -unordered_map~string,int~ wordFreqMap_
        -vector~DictionaryEntry~ allEntries_
        +ChineseDictionaryReader()
        +ChineseDictionaryReader(string dictPath)
        +loadDictionary(string dictPath) bool
        +getWordFrequency(string word) int
        +getAllDictionaryEntries() vector~DictionaryEntry~
        +getDictionarySize() size_t
    }

    class EnglishDictionaryReader {
        -unordered_map~string,int~ wordFreqMap_
        -vector~DictionaryEntry~ allEntries_
        +EnglishDictionaryReader()
        +EnglishDictionaryReader(string dictPath)
        +loadDictionary(string dictPath) bool
        +getWordFrequency(string word) int
        +getAllDictionaryEntries() vector~DictionaryEntry~
        +getDictionarySize() size_t
    }

    class IndexReader {
        <<abstract>>
        +~IndexReader()
        +loadIndex(string indexPath) bool
        +getLineNumbers(string character) vector~int~
        +getAllIndexEntries() vector~IndexEntry~
    }

    class ChineseIndexReader {
        -unordered_map~string,vector~int~~ indexMap_
        -vector~IndexEntry~ allEntries_
        +ChineseIndexReader()
        +ChineseIndexReader(string indexPath)
        +loadIndex(string indexPath) bool
        +getLineNumbers(string character) vector~int~
        +getAllIndexEntries() vector~IndexEntry~
    }

    class EnglishIndexReader {
        -unordered_map~char,vector~int~~ indexMap_
        -vector~IndexEntry~ allEntries_
        +EnglishIndexReader()
        +EnglishIndexReader(string indexPath)
        +loadIndex(string indexPath) bool
        +getLineNumbers(string character) vector~int~
        +getAllIndexEntries() vector~IndexEntry~
    }

    class WebPageLibraryReader {
        -vector~WebPageOffset~ offsetsList_
        -unordered_map~string,InvertedIndexEntry~ invertedIndex_
        +loadWebPages(string webpagesPath) bool
        +loadOffsets(string offsetsPath) bool
        +loadInvertedIndex(string indexPath) bool
        +getWebPageByDocid(int docid) WebPage
        +getInvertedIndexEntry(string term) InvertedIndexEntry
        +getOffsetsSize() size_t
        +getInvertedIndexSize() size_t
    }

    %% ===== 分词和文本处理层 =====
    class UnifiedTokenizer {
        -unique_ptr~EnglishTokenizer~ englishTokenizer_
        -unique_ptr~ChineseTokenizer~ chineseTokenizer_
        +UnifiedTokenizer(string enStopwordsFile, string cnStopwordsFile)
        +~UnifiedTokenizer()
        +processDirectories(string enDir, string cnDir, string enDict, string enIndex, string cnDict, string cnIndex) bool
        +processEnglishDirectory(string enDir, string enDict, string enIndex) bool
        +processChineseDirectory(string cnDir, string cnDict, string cnIndex) bool
        +getEnglishUniqueWords() size_t
        +getChineseUniqueWords() size_t
        +getTotalProcessedFiles() size_t
        +printAllStatistics() void
        +printEnglishStatistics() void
        +printChineseStatistics() void
    }

    class EnglishTokenizer {
        -unique_ptr~StopWordsManager~ stopWordsManager_
        -unordered_map~string,int~ wordFrequency_
        -map~char,vector~int~~ letterIndex_
        -size_t totalWords_
        -size_t validWords_
        -size_t stopWordsFiltered_
        -size_t processedFiles_
        +EnglishTokenizer(string stopwordsFile)
        +processDirectory(string inputDir, string dictFile, string indexFile, string extension) bool
        +processFiles(vector~string~ inputFiles, string dictFile, string indexFile) bool
        +getTotalWords() size_t
        +getUniqueWords() size_t
        +getValidWords() size_t
        +printStatistics() void
        -processFile(string filePath) bool
        -processLine(string line) vector~string~
        -addWord(string word) void
        -buildLetterIndex() void
        -saveDictionary(string dictFile) bool
        -saveIndex(string indexFile) bool
    }

    class ChineseTokenizer {
        -unique_ptr~StopWordsManager~ stopWordsManager_
        -unique_ptr~cppjieba::Jieba~ jieba_
        -unordered_map~string,int~ wordFrequency_
        -map~string,vector~int~~ characterIndex_
        -size_t totalWords_
        -size_t validWords_
        -size_t stopWordsFiltered_
        -size_t processedFiles_
        +ChineseTokenizer(string stopwordsFile)
        +processDirectory(string inputDir, string dictFile, string indexFile, string extension) bool
        +processFiles(vector~string~ inputFiles, string dictFile, string indexFile) bool
        +segmentText(string text) vector~string~
        +getTotalWords() size_t
        +getUniqueWords() size_t
        +getValidWords() size_t
        +getStopWordsFiltered() size_t
        +getProcessedFiles() size_t
        +printStatistics() void
        -processFile(string filePath) bool
        -processLine(string line) vector~string~
        -addWord(string word) void
        -buildCharacterIndex() void
        -saveDictionary(string dictFile) bool
        -saveIndex(string indexFile) bool
        -resetStatistics() void
        -extractAllCharacters(string word) vector~string~
    }

    class EnglishTextPreprocessor {
        +toLowerCase(string text) string
        +removePunctuation(string text) string
        +isValidWord(string word) bool
        +splitIntoWords(string text) vector~string~
    }

    class ChineseTextPreprocessor {
        +isValidChineseWord(string word) bool
        +normalizeChineseText(string text) string
    }

    class StopWordsManager {
        -unordered_set~string~ stopWords_
        +StopWordsManager(string stopWordsFile)
        +isStopWord(string word) bool
        +getStopWordsCount() size_t
        -loadStopWords(string filePath) void
    }

    %% ===== 搜索引擎业务层 =====
    class WebSearchEngine {
        -DataReaderManager* dataManager_
        -LRUCache~SearchCacheKey,vector~SearchResult~~ searchCache_
        -int maxSummaryLength_
        +WebSearchEngine(DataReaderManager* dataManager)
        +~WebSearchEngine()
        +search(string query, int topN) vector~SearchResult~
        +searchToJson(string query, int topN) nlohmann::json
        +setSummaryLength(int length) void
        +setCacheCapacity(size_t searchCacheCapacity) void
        +getCacheStats() LRUCache::CacheStats
        +clearCache() void
        -performSearch(vector~string~ terms, int topN) vector~SearchResult~
        -calculateCosineSimilarity(vector~string~ queryTerms, int docid, unordered_map~string,double~ queryVector) double
        -buildQueryVector(vector~string~ queryTerms) unordered_map~string,double~
        -normalizeVector(unordered_map~string,double~ vector) void
        -generateSummary(string content, vector~string~ queryTerms) string
        -highlightKeywords(string text, vector~string~ keywords) string
        -extractSentencesWithKeywords(string content, vector~string~ keywords, int maxLength) string
    }

    class KeywordRecommender {
        -DataReaderManager* dataManager_
        -unique_ptr~TextProcessor~ textProcessor_
        -LRUCache~RecommendationCacheKey,vector~Candidate~~ recommendCache_
        -LRUCache~EditDistanceCacheKey,int~ editDistanceCache_
        -int maxEditDistance_
        +KeywordRecommender(DataReaderManager* dataManager)
        +~KeywordRecommender()
        +recommendKeywords(string query, int k) vector~Candidate~
        +recommendToJson(string query, int k) nlohmann::json
        +setMaxEditDistance(int distance) void
        +setCacheCapacity(size_t recCap, size_t editCap) void
        +getCacheStats() pair~LRUCache::CacheStats,LRUCache::CacheStats~
        +clearCache() void
        -performRecommendation(string query, int k) vector~Candidate~
        -calculateEditDistance(string word1, string word2) int
        -processQuery(string query) vector~string~
        -findCandidatesFromDictionary(string queryChar, vector~Candidate~ candidates) void
        -mergeCandidates(vector~Candidate~ candidates) vector~Candidate~
        -selectTopKCandidates(vector~Candidate~ candidates, int k) vector~Candidate~
    }

    class TextProcessor {
        +utf8Split(string input) vector~string~
        +isChinese(string text) bool
        +isEnglish(string text) bool
    }

    %% ===== 网页处理层 =====
    class PageProcessor {
        -unique_ptr~StopWordsManager~ stopWordsManager_
        -unique_ptr~cppjieba::Jieba~ jieba_
        -unique_ptr~simhash::Simhasher~ simhasher_
        -vector~WebPage~ uniqueWebPages_
        -unordered_set~uint64_t~ uniqueHashes_
        +PageProcessor(string stopwordsFile)
        +~PageProcessor()
        +parseXmlFile(string xmlPath) vector~WebPage~
        +parseAllXmlFiles(string xmlDir) vector~WebPage~
        +deduplicateWebPages(vector~WebPage~ webpages, int topk, int threshold) void
        +buildWebPagesAndOffsets(string outputPath, string offsetPath) bool
        +buildInvertedIndex(string outputPath) bool
        -cleanText(string rawText) string
        -removeCDATA(string text) string
        -removeHtmlTags(string text) string
        -trim(string text) string
        -parseRssFormat(XMLElement* channel, vector~WebPage~ webpages) void
        -tokenizeAndFilter(string content) vector~string~
        -calculateTfIdf(vector~WebPage~ webpages) unordered_map~string,unordered_map~int,double~~
        -calculateTermFrequency(vector~string~ tokens) unordered_map~string,int~
        -calculateInverseDocumentFrequency(vector~WebPage~ webpages) unordered_map~string,double~
        -generateInvertedIndexFile(string outputPath, unordered_map~string,unordered_map~int,double~~ tfidfWeights) bool
    }

    %% ===== 配置管理层 =====
    class Configuration {
        -string filepath_
        -map~string,string~ configMap_
        -set~string~ stopWordList_
        +Configuration(string filepath)
        +getConfigMap() const map~string,string~&
        +getStopWordList() const set~string~&
        +getString(string key) string
        +getInt(string key) int
        +isStopWord(string word) bool
        +printConfig() void
        -loadConfigFile() void
        -loadStopWordsFile() void
        -initializeDefaults() void
        -trim(string str) string
    }

    %% ===== 缓存系统 =====
    class LRUCache~Key,Value~ {
        <<template>>
        -size_t capacity_
        -list~CacheNode~ cacheList_
        -unordered_map~Key,ListIterator~ cacheMap_
        -mutable mutex cacheMutex_
        -mutable size_t hitCount_
        -mutable size_t missCount_
        +LRUCache(size_t capacity)
        +~LRUCache()
        +get(Key key, Value value) bool
        +put(Key key, Value value) void
        +setCapacity(size_t capacity) void
        +clear() void
        +size() size_t
        +capacity() size_t
        +getStats() CacheStats
    }

    class CacheNode~Key,Value~ {
        +Key key
        +Value value
        +CacheNode(Key k, Value v)
    }

    class CacheStats {
        +size_t hitCount
        +size_t missCount
        +size_t totalRequests
        +double hitRate
        +CacheStats()
    }

    %% ===== 协议处理层 =====
    class TLVCodec {
        +encodeMessage(TLVMessage message) vector~uint8_t~
        +decodeMessage(vector~uint8_t~ buffer) TLVMessage
        +parseHeader(vector~uint8_t~ buffer) TLVHeader
        +validateMessage(TLVMessage message) bool
    }

    class TLVMessageBuilder {
        +buildKeywordRecommendRequest(string query, int k) TLVMessage
        +buildSearchRequest(string query, int topN) TLVMessage
        +buildKeywordRecommendResponse(vector~Candidate~ candidates) TLVMessage
        +buildSearchResponse(vector~SearchResult~ results) TLVMessage
        +buildErrorResponse(string errorMsg, int errorCode) TLVMessage
    }

    %% ===== 工具类 =====
    class DirectoryUtils {
        +getFilesInDirectory(string dirPath, string extension) vector~string~
        +isDirectory(string path) bool
        +fileExists(string filePath) bool
        -hasExtension(string filename, string extension) bool
    }

    %% ===== 数据结构和枚举 =====
    class MessageType {
        <<enumeration>>
        KEYWORD_RECOMMEND_REQUEST = 0x0001
        SEARCH_REQUEST = 0x0002
        KEYWORD_RECOMMEND_RESPONSE = 0x1001
        SEARCH_RESPONSE = 0x1002
        ERROR_RESPONSE = 0x9001
    }

    class TLVHeader {
        +uint16_t type
        +uint32_t length
        +TLVHeader(MessageType msgType, uint32_t dataLen)
    }

    class TLVMessage {
        +TLVHeader header
        +vector~uint8_t~ data
        +TLVMessage()
        +TLVMessage(MessageType msgType, string jsonData)
        +getType() MessageType
        +getJsonData() string
        +getTotalLength() size_t
    }

    class WebPage {
        +int docid
        +string link
        +string title
        +string content
        +WebPage(int id)
    }

    class SearchResult {
        +int docid
        +string title
        +string url
        +string summary
        +double score
        +SearchResult(int id, string title, string url, string summary, double score)
        +toJson() nlohmann::json
    }

    class Candidate {
        +string word
        +int editDistance
        +int frequency
        +Candidate(string word, int ed, int freq)
        +toJson() nlohmann::json
    }

    class DictionaryEntry {
        +string word
        +int frequency
        +DictionaryEntry(string w, int freq)
    }

    class IndexEntry {
        +string character
        +vector~int~ lineNumbers
        +IndexEntry(string ch)
    }

    class WebPageOffset {
        +int docid
        +size_t offset
        +size_t length
        +WebPageOffset()
        +WebPageOffset(int id, size_t off, size_t len)
    }

    class InvertedIndexEntry {
        +string term
        +vector~pair~int,double~~ docWeights
        +InvertedIndexEntry(string t)
    }

    class WordInfo {
        +string word
        +int frequency
        +WordInfo(string w, int freq)
    }

    class DocumentPosition {
        +int doc_id
        +vector~int~ positions
        +DocumentPosition(int id)
    }

    class SearchCacheKey {
        +string query
        +int topN
        +operator==(SearchCacheKey other) bool
        +operator<(SearchCacheKey other) bool
    }

    class RecommendationCacheKey {
        +string query
        +int k
        +operator==(RecommendationCacheKey other) bool
        +operator<(RecommendationCacheKey other) bool
    }

    class EditDistanceCacheKey {
        +string word1
        +string word2
        +operator==(EditDistanceCacheKey other) bool
        +operator<(EditDistanceCacheKey other) bool
    }

    class CandidateComparator {
        +operator()(Candidate a, Candidate b) bool
    }

    class SearchResultComparator {
        +operator()(SearchResult a, SearchResult b) bool
    }

    %% ===== 继承关系 =====
    ChineseDictionaryReader --|> DictionaryReader : inherits
    EnglishDictionaryReader --|> DictionaryReader : inherits
    ChineseIndexReader --|> IndexReader : inherits
    EnglishIndexReader --|> IndexReader : inherits
    Socket --|> NonCopyable : inherits

    %% ===== 组合关系 =====
    SearchEngineServer *-- TcpServer : contains
    SearchEngineServer *-- ThreadPool : contains
    SearchEngineServer *-- DataReaderManager : contains
    
    TcpServer *-- Acceptor : contains
    TcpServer *-- EventLoop : contains
    
    ThreadPool *-- TaskQueue : contains
    
    Acceptor *-- Socket : contains
    Acceptor *-- InetAddress : contains
    
    DataReaderManager *-- ChineseDictionaryReader : contains
    DataReaderManager *-- EnglishDictionaryReader : contains
    DataReaderManager *-- ChineseIndexReader : contains
    DataReaderManager *-- EnglishIndexReader : contains
    DataReaderManager *-- WebPageLibraryReader : contains
    
    UnifiedTokenizer *-- EnglishTokenizer : contains
    UnifiedTokenizer *-- ChineseTokenizer : contains
    
    EnglishTokenizer *-- StopWordsManager : contains
    ChineseTokenizer *-- StopWordsManager : contains
    PageProcessor *-- StopWordsManager : contains
    
    KeywordRecommender *-- TextProcessor : contains
    
    WebSearchEngine *-- LRUCache : contains
    KeywordRecommender *-- LRUCache : contains
    
    LRUCache *-- CacheNode : contains
    
    TLVMessage *-- TLVHeader : contains

    %% ===== 聚合关系 =====
    SearchEngineTask o-- DataReaderManager : uses
    WebSearchEngine o-- DataReaderManager : uses
    KeywordRecommender o-- DataReaderManager : uses
    
    %% ===== 依赖关系 =====
    SearchEngineServer ..> TcpConnection : uses
    SearchEngineServer ..> SearchEngineTask : creates
    
    SearchEngineTask ..> WebSearchEngine : uses
    SearchEngineTask ..> KeywordRecommender : uses
    SearchEngineTask ..> TLVMessage : uses
    SearchEngineTask ..> TLVCodec : uses
    
    WebSearchEngine ..> SearchResult : creates
    KeywordRecommender ..> Candidate : creates
    PageProcessor ..> WebPage : creates
    
    TLVMessageBuilder ..> TLVMessage : creates
    TLVCodec ..> TLVMessage : processes
    
    EnglishTokenizer ..> WordInfo : uses
    EnglishTokenizer ..> DocumentPosition : uses
    
    WebSearchEngine ..> SearchCacheKey : uses
    KeywordRecommender ..> RecommendationCacheKey : uses
    KeywordRecommender ..> EditDistanceCacheKey : uses
    
    KeywordRecommender ..> CandidateComparator : uses
    WebSearchEngine ..> SearchResultComparator : uses
```

## 使用方法

1. **复制完整代码**: 将上述Mermaid代码复制到支持Mermaid的编辑器中
2. **在线渲染**: 访问 [Mermaid Live Editor](https://mermaid.live/) 粘贴代码查看图表
3. **GitHub渲染**: 在GitHub的Markdown文件中会自动渲染
4. **本地使用**: 配合支持Mermaid的文档工具使用

## 图表特点

- **完整性**: 包含项目中所有主要类和关系
- **详细性**: 列出了重要的成员变量和方法
- **层次性**: 按功能模块分组，便于理解
- **关系性**: 明确标示了继承、组合、聚合、依赖关系
- **规范性**: 遵循项目的命名规范（成员变量后缀下划线）
</rewritten_file> 