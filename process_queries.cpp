#include "process_queries.h"
#include <execution>
#include <algorithm>

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
	vector<vector<Document>> result(queries.size());
	
	transform(execution::par,
			  queries.begin(), queries.end(),
			  result.begin(),
			  [&search_server](string s) { return search_server.FindTopDocuments(s); }
			);
	
	return result;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
	vector<Document> result;
	
	for(const auto doc : ProcessQueries(search_server, queries)) {
		copy(execution::par,
			 doc.begin(), doc.end(),
			 back_inserter(result)
			);
	}
	
	return result;
}