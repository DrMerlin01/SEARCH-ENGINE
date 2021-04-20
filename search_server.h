#pragma once

#include <map>
#include <vector>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <cmath>
#include <execution>
#include <type_traits>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
	explicit SearchServer(const std::string& stop_words_text);

	template <typename StringContainer>
	explicit SearchServer(StringContainer stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) { // Extract non-empty stop words
		if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
			throw std::invalid_argument("Some of stop words are invalid");
		}
	}

	void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
		return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
	}

	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	template <typename  ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
		const auto query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(policy, query, document_predicate);
		
		std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			} else {
				return lhs.relevance > rhs.relevance;
			}
		}); 
		
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}

		return matched_documents;
	}

	template<typename  ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const {
		return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
			return document_status == status;
		});
	}

	template<typename  ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const {
		return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
	}

	int GetDocumentCount() const;

	auto begin() const {
		return document_ids_.begin();
	}

	auto end() const {
		return document_ids_.end();
	}

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
	
	void RemoveDocument(int document_id);

	template<typename  ExecutionPolicy>
	void RemoveDocument(const ExecutionPolicy& policy, int document_id) {
		document_ids_.erase(document_id);
		documents_.erase(document_id);
		word_to_document_freqs_on_id_.erase(document_id);
		for_each(policy,
				 word_to_document_freqs_.begin(), word_to_document_freqs_.end(),
				 [&document_id](auto& content) {
					content.second.erase(document_id);
				});
	}

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

	template<typename  ExecutionPolicy>
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const ExecutionPolicy& policy, std::string_view raw_query, int document_id) const {
		const auto query = ParseQuery(raw_query);
		std::vector<std::string_view> matched_words;
		
		for_each(policy,
				 query.plus_words.begin(), query.plus_words.end(),
				 [&matched_words, &document_id, this](const std::string_view word) {
					if (word_to_document_freqs_.count(word) == 0) {
						return;
					}
					if (word_to_document_freqs_.find(word)->second.count(document_id)) {
						matched_words.push_back(word);
					}
				});
		for_each(policy,
				 query.minus_words.begin(), query.minus_words.end(),
				 [&matched_words, &document_id, this](const std::string_view word) {
					if (word_to_document_freqs_.count(word) == 0) {
						return;
					}
					if (word_to_document_freqs_.find(word)->second.count(document_id)) {
						matched_words.clear();
					}
				});
		
		return {matched_words, documents_.at(document_id).status};
	}

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	const std::set<std::string, std::less<>> stop_words_;
	std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
	std::map<int, std::map<std::string_view, double>> word_to_document_freqs_on_id_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;

	bool IsStopWord(std::string_view word) const;

	static bool IsValidWord(std::string_view word);

	std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string_view text) const;

	struct Query {
		std::set<std::string_view> plus_words;
		std::set<std::string_view> minus_words;
	};

	Query ParseQuery(std::string_view text) const;
	// Existence required
	double ComputeWordInverseDocumentFreq(std::string_view word) const;

	template <typename  ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query, DocumentPredicate document_predicate) const {
		ConcurrentMap<int, double> document_to_relevance(3);
		
		for_each(policy,
				 query.plus_words.begin(), query.plus_words.end(),
				 [&document_to_relevance, &document_predicate, this](const std::string_view word) {
					if (word_to_document_freqs_.count(word) == 0) {
						return;
					}
					const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
					for (const auto [document_id, term_freq] : word_to_document_freqs_.find(word)->second) {
						const auto& document_data = documents_.at(document_id);
						if (document_predicate(document_id, document_data.status, document_data.rating)) {
							document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
						}
					}
				});
		
		for_each(policy,
				 query.minus_words.begin(), query.minus_words.end(),
				 [&document_to_relevance, this](const std::string_view word) {
					if (word_to_document_freqs_.count(word) == 0) {
						return;
					}
					for (const auto [document_id, _] : word_to_document_freqs_.find(word)->second) {
						document_to_relevance.erase(document_id);
					}
				});

		std::vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
			matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
		}
		return matched_documents;
	}
};