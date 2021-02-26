#include "remove_duplicates.h"
#include <iostream>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	map<set<string>, int> documents_content;
	vector<int> list_remove;
	for(const int document_id : search_server) {
		const map<string, double> words = search_server.GetWordFrequencies(document_id);
		set<string> content;
		for(const auto [word, freq] : words) {
			content.insert(word);
		}
		if(documents_content.count(content) == 0) {
			documents_content[content] = document_id;
		} else {
			list_remove.push_back(document_id);
		}
	}

	for(const auto document_id : list_remove) {
		search_server.RemoveDocument(document_id);
		cout << "Found duplicate document id "s << document_id << endl;
	}
}