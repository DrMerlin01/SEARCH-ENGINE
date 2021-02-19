#include "test_example_functions.h"

using namespace std;

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
	search_server.AddDocument(document_id, document, status, ratings);
}
