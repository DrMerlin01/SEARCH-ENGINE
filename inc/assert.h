#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <ostream>

using namespace std;

template <typename First, typename Second>
ostream& operator<<(ostream& out, const pair<First, Second>& container) {
	return out << container.first << ": "s << container.second;
}

template <typename Container>
ostream& Print(ostream& out, const Container& container) {
	bool first = true;
	for (const auto& element : container) {
		if(first) {
			first = false;
		} else {
			out << ", "s;
		}

		out << element;
	}

	return out;
}

template <typename Term>
ostream& operator<<(ostream& out, const vector<Term>& container) {
	out << "["s;
	Print(out, container);
	out << "]"s;

	return out;
}

template <typename Term>
ostream& operator<<(ostream& out, const set<Term>& container) {
	out << "{"s;
	Print(out, container);
	out << "}"s;

	return out;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
	out << "{"s;
	Print(out, container);
	out << "}"s;

	return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cerr << boolalpha;
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cerr << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint) {
	if (!value) {
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func>
void RunTestImpl(Func func, const string& name_func) {
	func();
	cerr << name_func << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)