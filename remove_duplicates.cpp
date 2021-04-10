#include "remove_duplicates.h"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	map<set<string>, int> document_content;
	vector<int> list_remove;
	
	for (const int document_id : search_server) {
		set<string> content;
		for (const auto [word, freq] : search_server.GetWordFrequencies(document_id)) {
			content.insert(word);
		}
		if (document_content.count(content) == 0) {
			document_content.insert({content, document_id});
		} else {
			list_remove.push_back(document_id);
		}
	}

	for (const auto document_id : list_remove) {
		search_server.RemoveDocument(document_id);
		cout << "Found duplicate document id "s << document_id << endl;
	}
}