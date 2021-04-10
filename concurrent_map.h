#pragma once

#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

	struct Bucket {
		std::mutex mutex;
		std::map<Key, Value> map;
	}; 

	struct Access {
		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;
		
		Access(const Key& key, Bucket& bucket)
			: guard(bucket.mutex)
			, ref_to_value(bucket.map[key])
		{
		}
	};

	explicit ConcurrentMap(size_t bucket_count)
		: bucket_count_(bucket_count)
		, buckets_(bucket_count)
	{
	}

	Access operator[](const Key& key) {
		return Access{key, buckets_[CalculatePosition(key)]};
	}
	
	void erase(const Key& key) {
		buckets_[CalculatePosition(key)].map.erase(key);
	}

	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> result;
		for(auto& bucket : buckets_) {
			std::lock_guard guard(bucket.mutex);
			for(const auto& [key, value] : bucket.map) {
				result[key] = value;
			}
		}
		return result;
	}

private:
	size_t bucket_count_ = 0;
	std::vector<Bucket> buckets_;
	
	uint64_t CalculatePosition(Key key) {
		uint64_t position = key;
		return position % bucket_count_;
	}
};