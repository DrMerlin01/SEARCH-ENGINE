#include "process_queries.h"
#include <execution>
#include <algorithm>

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
	vector<vector<Document>> results(queries.size());
	
	transform(execution::par,
			  queries.begin(), queries.end(),
			  results.begin(),
			  [&search_server](const string& query) { return search_server.FindTopDocuments(query); }
			);
	
	return results;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
	vector<Document> result;
	
	for(const auto& documents : ProcessQueries(search_server, queries)) {
		move(execution::par,
			 documents.begin(), documents.end(),
			 back_inserter(result)
			);
	}
	
	return result;
}