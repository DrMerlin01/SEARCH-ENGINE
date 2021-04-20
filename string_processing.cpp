#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
	vector<string_view> words;
	const auto pos_end = text.npos;  
	while (true) {
		auto space = text.find(' ', 0);
		words.push_back(text.substr(0, space));
		if (space == pos_end) {
			break;
		} else {
			text.remove_prefix(space + 1);
		}
	}
	
	return words;
}