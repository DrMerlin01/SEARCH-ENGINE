#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

template <typename First, typename Second>
ostream& operator<<(ostream& out, const pair<First, Second>& container) {
    out << container.first << ": "s << container.second;
    return out;
}

template <typename num>
ostream& Print(ostream& out, const num& container) {
    bool first = true;
    for (const auto& element : container) {
        if(first) {
            out << element;
            first = false;
        } else {
            out << ", "s << element;
        }

    }
    return out;
}

template <typename Term>
ostream& operator<<(ostream& out, const vector<Term>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template <typename Term>
ostream& operator<<(ostream& out, const set<Term>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
    public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                               ComputeAverageRating(ratings),
                               status
                               });
    }

    template <typename KeyPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, KeyPredicate key_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

    private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
            };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename KeyPredicate>
    vector<Document> FindAllDocuments(const Query& query, KeyPredicate key_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const DocumentData& document = documents_.at(document_id);
                if (key_predicate(document_id, document.status, document.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }

            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func>
void RunTestImpl(Func func, const string& name_func) {
    func();
    cerr << name_func << " OK" << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestExcludeDocumentsWithMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "oh my my in"s;
    const vector<int> ratings_1 = {4, 5, 6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        ASSERT_EQUAL(server.FindTopDocuments("in city"s).size(), 2);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        ASSERT_EQUAL(server.FindTopDocuments("in -city"s).size(), 1);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        ASSERT_HINT(server.FindTopDocuments("cat -city"s).empty(), "Documents with minus words must be excluded"s);
    }
}

void TestComputeRatings() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "oh my my in"s;
    const vector<int> ratings_1 = {4, 5, 6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs = server.FindTopDocuments("in -city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, 5);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        ASSERT(server.FindTopDocuments("-in"s).empty());
    }
}

void TestIncludeFindedDocumentsWithStatus() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "oh my my in"s;
    const vector<int> ratings_1 = {4, 5, 6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        ASSERT_EQUAL(server.FindTopDocuments("in"s).size(), 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(server.FindTopDocuments("in"s, DocumentStatus::BANNED).size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_1);
    }
}

void TestIncludeFindedDocumentsWithPredicate() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "oh my my in"s;
    const vector<int> ratings_1 = {4, 5, 6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        ASSERT_EQUAL(server.FindTopDocuments("in"s).size(), 2);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        const auto found_docs = server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 42 == 0; });
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        const auto found_docs = server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { return rating > 4; });
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_1);
    }
}

void TestSortByRelevance() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_1 = {4, 5, 6};

    const int doc_id_2 = 2;
    const string content_2 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings_2 = {-1, 12, -6};

    {
        SearchServer server;
        server.SetStopWords("и в на");
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_HINT(doc0.relevance >= doc1.relevance, "Must be sort by descending order"s);
    }
}

void TestComputeRelevance() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_1 = {4, 5, 6};

    const int doc_id_2 = 2;
    const string content_2 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings_2 = {-1, 12, -6};

    {
        SearchServer server;
        server.SetStopWords("и в на");
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        const Document& doc2 = found_docs[2];
        ASSERT(doc0.relevance >= 0.650672);
        ASSERT(doc1.relevance >= 0.274653);
        ASSERT(doc2.relevance >= 0.101366);
    }
}

void TestMatchDocuments() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_1 = 48;
    const string content_1 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_1 = {4, 5, 6};

    const int doc_id_2 = 2;
    const string content_2 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings_2 = {-1, 12, -6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto [words, status] = server.MatchDocument("пушистый ухоженный кот"s, 48);
        ASSERT_EQUAL(words.size(), 2);
        const string str = words[0];
        const string str2 = words[1];
        ASSERT_EQUAL_HINT(str, "кот"s, "Words sort by alphabet"s);
        ASSERT_EQUAL_HINT(str2, "пушистый"s, "Words sort by alphabet"s);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto [words, status] = server.MatchDocument("пушистый ухоженный пес"s, 42);
        ASSERT_HINT(words.empty(), "Result must be empty, because words not found"s);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestComputeRatings);
    RUN_TEST(TestComputeRelevance);
    RUN_TEST(TestIncludeFindedDocumentsWithStatus);
    RUN_TEST(TestIncludeFindedDocumentsWithPredicate);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestMatchDocuments);
}

int main() {
    TestSearchServer();
    cerr << "Search server testing finished"s << endl;
}
