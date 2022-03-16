#pragma once

#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap {
private:
	struct Bucket {
		std::mutex mutex;
		std::map<Key, Value> map;
	};
	std::vector<Bucket> buckets_;

	uint64_t CalculatePosition(const Key& key) const {
		const uint64_t position = key;

		return position % buckets_.size();
	}

	Bucket& FindBucket(const Key& key) {
		return buckets_[CalculatePosition(key)];
	}

public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"); 

	struct Access {
		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;

		Access(const Key& key, Bucket& bucket)
			: guard(bucket.mutex)
			, ref_to_value(bucket.map[key]) {
		}
	};

	explicit ConcurrentMap(size_t bucket_count)
		: buckets_(bucket_count) {
	}

	Access operator[](const Key& key) {
		return Access{key, FindBucket(key)};
	}

	void erase(const Key& key) {
		Bucket& bucket = FindBucket(key); 
		std::lock_guard guard(bucket.mutex);
		bucket.map.erase(key);
	}

	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> result;
		for(auto& [mutex, map] : buckets_) {
			std::lock_guard guard(mutex);
			result.insert(map.begin(), map.end());
		}

		return result;
	}
};