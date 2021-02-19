#include "remove_duplicates.h"
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	map<int, set<string>> document_content;
	set<int> list_remove;
	for(const int document_id : search_server) {
		map<string, double> words = search_server.GetWordFrequencies(document_id);
		for(auto it = words.begin(); it != words.end(); ++it) {
			document_content[document_id].insert(it->first);
		}
	}

	for(auto it = document_content.begin(); it != document_content.end(); ++it) {
		for(auto iter = document_content.begin(); iter != document_content.end(); ++iter) {
			if(it->second == iter->second && it->first < iter->first) {
				list_remove.insert(iter->first);
			}
		}
	}

	for(const auto document_id : list_remove) {
		search_server.RemoveDocument(document_id);
		cout << "Found duplicate document id "s << document_id << endl;
	}
}