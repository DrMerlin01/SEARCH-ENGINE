#include "search_server.cpp"
#include "assert.cpp"

// -------- Начало модульных тестов поисковой системы ----------
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1U);
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
	SearchServer server;
	server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	server.AddDocument(48, "oh my my in"s, DocumentStatus::ACTUAL, {4, 5, 6});
	ASSERT_EQUAL(server.FindTopDocuments("in city"s).size(), 2U);
	ASSERT_EQUAL(server.FindTopDocuments("in -city"s).size(), 1U);
	ASSERT_HINT(server.FindTopDocuments("cat -city"s).empty(), "Documents with minus words must be excluded"s);
}

void TestComputeRatings() {
	SearchServer server;
	server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	server.AddDocument(48, "oh my my in"s, DocumentStatus::ACTUAL, {4, 5, 6});
	const auto found_docs = server.FindTopDocuments("in -city"s);
	ASSERT_EQUAL(found_docs.size(), 1U);
	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL(doc0.rating, 5);
}

void TestIncludeFindedDocumentsWithStatus() {
	SearchServer server;
	server.AddDocument(42, "cat in the city"s, DocumentStatus::IRRELEVANT, {1, 2, 3});
	server.AddDocument(48, "oh my my in"s, DocumentStatus::BANNED, {4, 5, 6});

	{
		ASSERT_EQUAL(server.FindTopDocuments("in"s).size(), 0U);
	}

	{
		const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED);
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
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		ASSERT_EQUAL(server.FindTopDocuments("in"s).size(), 2U);
	}

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
		const auto found_docs = server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 42 == 0; });
		ASSERT(found_docs.size() > 0U);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
		const auto found_docs = server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { return rating > 4; });
		ASSERT(found_docs.size() > 0U);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id_1);
	}
}

void TestSortByRelevance() {
	SearchServer server;
	server.SetStopWords("и в на");
	server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});
	const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
	ASSERT_EQUAL(found_docs.size(), 3U);
	const Document& doc0 = found_docs[0];
	const Document& doc1 = found_docs[1];
	const Document& doc2 = found_docs[2];
	ASSERT_HINT(doc0.relevance >= doc1.relevance, "Must be sort by descending order"s);
	ASSERT_HINT(doc1.relevance >= doc2.relevance, "Must be sort by descending order"s);
}

void TestComputeRelevance() {
	SearchServer server;
	server.SetStopWords("и в на");
	server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});
	const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
	ASSERT_EQUAL(found_docs.size(), 3U);
	const Document& doc0 = found_docs[0];
	const Document& doc1 = found_docs[1];
	const Document& doc2 = found_docs[2]; 
	ASSERT(abs(doc0.relevance - 0.650672) < 1e-6);
	ASSERT(abs(doc1.relevance - 0.274653) < 1e-6);
	ASSERT(abs(doc2.relevance - 0.101366) < 1e-6);
}

void TestMatchDocuments() {	 
	SearchServer server;
	server.AddDocument(42, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {1, 2, 3});
	server.AddDocument(48, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {4, 5, 6});
	server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::BANNED, {-1, 12, -6});

	{
		const auto [words, status] = server.MatchDocument("пушистый ухоженный кот"s, 48);
		ASSERT_EQUAL(words.size(), 2U);
		ASSERT_EQUAL_HINT(words[0], "кот"s, "Words sort by alphabet"s);
		ASSERT_EQUAL_HINT(words[1], "пушистый"s, "Words sort by alphabet"s);
	}

	{
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