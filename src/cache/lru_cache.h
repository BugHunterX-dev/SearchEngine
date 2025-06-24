#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <unordered_map>

template <typename Key, typename Value>
class LRUCache {
public:
    // 缓存节点
    struct CacheNode {
        Key key;
        Value value;

        CacheNode(const Key& k, const Value& v)
            : key(k)
            , value(v) {}
    };

    using ListIterator = typename std::list<CacheNode>::iterator;

private:
    size_t capacity_;                                // 缓存容量
    std::list<CacheNode> cacheList_;                 // 双向链表，头部是最新的
    std::unordered_map<Key, ListIterator> cacheMap_; // 快速索引
    mutable std::mutex cacheMutex_;                  // 线程安全

    // 统计信息
    mutable size_t hitCount_;  // 命中次数
    mutable size_t missCount_; // 未命中次数

public:
    explicit LRUCache(size_t capacity = 1000)
        : capacity_(capacity)
        , hitCount_(0)
        , missCount_(0) {}

    ~LRUCache() = default;

    // 获取值
    bool get(const Key& key, Value& value) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheMap_.find(key);
        if (it == cacheMap_.end()) {
            ++missCount_;
            return false;
        }

        // 移动到链表头部（最近使用）
        cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
        value = it->second->value;
        ++hitCount_;
        return true;
    }

    // 设置值
    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end()) {
            // 更新现有值
            it->second->value = value;
            // 移动到头部
            cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
            return;
        }

        // 检查容量，需要淘汰最旧的元素
        if (cacheList_.size() >= capacity_) {
            // 删除最旧的元素（链表尾部）
            const Key& oldKey = cacheList_.back().key;
            cacheMap_.erase(oldKey);
            cacheList_.pop_back();
        }

        // 添加新元素到头部
        cacheList_.emplace_front(key, value);
        cacheMap_[key] = cacheList_.begin();
    }

    // 删除元素
    bool remove(const Key& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheMap_.find(key);
        if (it == cacheMap_.end()) {
            return false;
        }

        cacheList_.erase(it->second);
        cacheMap_.erase(it);
        return true;
    }

    // 清空缓存
    void clear() {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cacheList_.clear();
        cacheMap_.clear();
        hitCount_ = 0;
        missCount_ = 0;
    }

    // 获取缓存统计信息
    struct CacheStats {
        size_t size;
        size_t capacity;
        size_t hitCount;
        size_t missCount;
        double hitRate;
    };

    CacheStats getStats() const {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        double hitRate = 0.0;
        size_t totalRequests = hitCount_ + missCount_;
        if (totalRequests > 0) {
            hitRate = static_cast<double>(hitCount_) / totalRequests;
        }

        return {cacheList_.size(), capacity_, hitCount_, missCount_, hitRate};
    }

    // 设置容量
    void setCapacity(size_t newCapacity) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        capacity_ = newCapacity;

        // 如果新容量小于当前大小，需要删除多余元素
        while (cacheList_.size() > capacity_) {
            const Key& oldKey = cacheList_.back().key;
            cacheMap_.erase(oldKey);
            cacheList_.pop_back();
        }
    }

    // 判断是否包含某个key
    bool contains(const Key& key) const {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        return cacheMap_.find(key) != cacheMap_.end();
    }

    // 获取当前大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        return cacheList_.size();
    }

    // 获取容量
    size_t capacity() const {
        return capacity_;
    }

    // 检查是否为空
    bool empty() const {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        return cacheList_.empty();
    }
};