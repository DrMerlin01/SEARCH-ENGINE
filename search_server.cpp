#include "search_server.h"
#include <atomic>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) { // Invoke delegating constructor
	// from string container
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0U)) {
		throw invalid_argument("Invalid document_id"s);
	}
	const auto words = SplitIntoWordsNoStop(document);

	const double inv_word_count = 1.0 / words.size();
	for (const string_view word : words) {
		auto result = all_words_.insert(static_cast<string>(word));
		word_to_document_freqs_[*result.first][document_id] += inv_word_count;
		word_to_document_freqs_on_id_[document_id][*result.first] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
	document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(execution::seq, raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
	return FindTopDocuments(execution::seq, raw_query);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const { 
	if(word_to_document_freqs_on_id_.count(document_id) == 0U) {        
		const static map<string_view, double> result;

		return result;
	} else {
		return word_to_document_freqs_on_id_.at(document_id);
	}
}

void SearchServer::RemoveDocument(int document_id) {
	RemoveDocument(execution::seq, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
	return MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, string_view raw_query, int document_id) const {
	const auto query = ParseQuery(raw_query);
	vector<string_view> matched_words;

	for (const string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0U) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}

	for (const string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0U) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}

	return {matched_words, documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, string_view raw_query, int document_id) const {
	const auto query = ParseQuery(raw_query);
	vector<string_view> matched_words(query.plus_words.size());
	atomic_int size = 0;

	for_each(execution::par,
			 query.plus_words.begin(), query.plus_words.end(),
			 [&matched_words, &size, &document_id, this](const string_view word) {
				 if (word_to_document_freqs_.count(word) == 0U) {
					 return;
				 }
				 if (word_to_document_freqs_.at(word).count(document_id)) {
					 matched_words[size++] = word;
				 }
			 });

	matched_words.resize(size);

	for_each(execution::par,
			 query.minus_words.begin(), query.minus_words.end(),
			 [&matched_words, &document_id, this](const string_view word) {
				 if (word_to_document_freqs_.count(word) == 0U) {
					 return;
				 }
				 if (!matched_words.empty() && word_to_document_freqs_.at(word).count(document_id)) {
					 matched_words.clear();

					 return;
				 }
			 });

	return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(string_view word) const {
	return stop_words_.count(word) > 0U;
}

bool SearchServer::IsValidWord(string_view word) {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
	});
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
	vector<string_view> words;
	for (const string_view word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw invalid_argument("Word "s + static_cast<string>(word) + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}

	return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	}

	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
	if (text.empty()) {
		throw invalid_argument("Query word is empty"s);
	}
	bool is_minus = false;
	if (text[0] == '-') {
		is_minus = true;
		text.remove_prefix(1);
	}
	if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
		throw invalid_argument("Query word "s + static_cast<string>(text) + " is invalid"s);
	}

	return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
	Query result;
	for (const string_view word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.insert(query_word.data);
			} else {
				result.plus_words.insert(query_word.data);
			}
		}
	}

	return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}