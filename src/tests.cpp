#include "../inc/tests.h"
#include "../inc/search_server.h"
#include "../inc/remove_duplicates.h"
#include "../inc/assert.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};

	{
		SearchServer search_server(""s);
		search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = search_server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1U);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}
	{
		SearchServer search_server("in the"s);
		search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(search_server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void TestExcludeDocumentsWithMinusWords() {
	SearchServer search_server(""s);
	search_server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(48, "oh my my in"s, DocumentStatus::ACTUAL, {4, 5, 6});
	ASSERT_EQUAL(search_server.FindTopDocuments("in city"s).size(), 2U);
	ASSERT_EQUAL(search_server.FindTopDocuments("in -city"s).size(), 1U);
	ASSERT_HINT(search_server.FindTopDocuments("cat -city"s).empty(), "Documents with minus words must be excluded"s);
}

void TestComputeRatings() {
	SearchServer search_server(""s);
	search_server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(48, "oh my my in"s, DocumentStatus::ACTUAL, {4, 5, 6});
	const auto found_docs = search_server.FindTopDocuments("in -city"s);
	ASSERT_EQUAL(found_docs.size(), 1U);
	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL(doc0.rating, 5);
}

void TestIncludeFindedDocumentsWithStatus() {
	SearchServer search_server(""s);
	search_server.AddDocument(42, "cat in the city"s, DocumentStatus::IRRELEVANT, {1, 2, 3});
	search_server.AddDocument(48, "oh my my in"s, DocumentStatus::BANNED, {4, 5, 6});

	{
		ASSERT_EQUAL(search_server.FindTopDocuments("in"s).size(), 0U);
	}
	{
		const auto found_docs = search_server.FindTopDocuments("in"s, DocumentStatus::BANNED);
		ASSERT_EQUAL(found_docs.size(), 1U);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, 48);
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
		SearchServer search_server(""s);
		search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		search_server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		ASSERT_EQUAL(search_server.FindTopDocuments("in"s).size(), 2U);
	}
	{
		SearchServer search_server(""s);
		search_server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
		search_server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
		const auto found_docs = search_server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 42 == 0; });
		ASSERT(found_docs.size() > 0U);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}
	{
		SearchServer search_server(""s);
		search_server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
		search_server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
		const auto found_docs = search_server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { return rating > 4; });
		ASSERT(found_docs.size() > 0U);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id_1);
	}
}

void TestSortByRelevance() {
	SearchServer search_server("и в на"s);
	search_server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	search_server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});
	const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
	ASSERT_EQUAL(found_docs.size(), 3U);
	const Document& doc0 = found_docs[0];
	const Document& doc1 = found_docs[1];
	const Document& doc2 = found_docs[2];
	ASSERT_HINT(doc0.relevance >= doc1.relevance, "Must be sort by descending order"s);
	ASSERT_HINT(doc1.relevance >= doc2.relevance, "Must be sort by descending order"s);
}

void TestComputeRelevance() {
	SearchServer search_server("и в на"s);
	search_server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	search_server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});
	const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
	ASSERT_EQUAL(found_docs.size(), 3U);
	const Document& doc0 = found_docs[0];
	const Document& doc1 = found_docs[1];
	const Document& doc2 = found_docs[2]; 
	ASSERT(abs(doc0.relevance - 0.650672) < 1e-6);
	ASSERT(abs(doc1.relevance - 0.274653) < 1e-6);
	ASSERT(abs(doc2.relevance - 0.101366) < 1e-6);
}

void TestMatchDocuments() {
	SearchServer search_server(""s);
	search_server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	search_server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});

	{
		const auto [words, status] = search_server.MatchDocument("ухоженный кот хвост"s, 48);
		ASSERT_EQUAL(words.size(), 2U);
		ASSERT_EQUAL_HINT(words[0], "кот"s, "Words sort by alphabet"s);
		ASSERT_EQUAL_HINT(words[1], "хвост"s, "Words sort by alphabet"s);
	}
	{
		const auto [words, status] = search_server.MatchDocument("пушистый ухоженный пес"s, 42);
		ASSERT_HINT(words.empty(), "Result must be empty, because words not found"s);
	}
}

void TestRemoveDuplicates() {
	SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    // дубликат документа 2, будет удалён
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    // отличие только в стоп-словах, считаем дубликатом
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    // множество слов такое же, считаем дубликатом документа 1
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    // добавились новые слова, дубликатом не является
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
    // есть не все слова, не является дубликатом
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    // слова из разных документов, не является дубликатом
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    ASSERT(search_server.GetDocumentCount() == 9);
    RemoveDuplicates(search_server);
    ASSERT(search_server.GetDocumentCount() == 5);
}

void TestRemoveDocument() {
	SearchServer search_server(""s);
	search_server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	search_server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});
	search_server.RemoveDocument(2);
	ASSERT(search_server.GetDocumentCount() == 2);
}

void TestGetWordFrequencies() {
	SearchServer search_server(""s);
	search_server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	search_server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});
	const auto words = search_server.GetWordFrequencies(48);
	ASSERT(words.at("хвост"sv) == 0.25);
	ASSERT(words.at("пушистый"sv) == 0.5);
	ASSERT(words.at("кот"sv) == 0.25);
}

void TestGetDocumentCount() {
	SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    ASSERT(search_server.GetDocumentCount() == 9);
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
	RUN_TEST(TestRemoveDuplicates);
	RUN_TEST(TestRemoveDocument);
	RUN_TEST(TestGetWordFrequencies);
	RUN_TEST(TestGetDocumentCount);

	cout << endl;
}