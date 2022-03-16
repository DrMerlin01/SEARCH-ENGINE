#include "../inc/test_example_functions.h"

#include <iostream>
#include <stdexcept>

using namespace std;

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
	try {
		search_server.AddDocument(document_id, document, status, ratings);
	} catch (const exception& e) {
		cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
	}
}
