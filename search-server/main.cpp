#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;

    for (const char c : text) {

        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }

        else {
            word += c;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

struct Query {
    set<string> plus_words;
    set<string> minus_words;
};

struct QueryWord {
    string word;
    bool is_plus;
    bool is_minus;
};

class SearchServer {
public:

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (string word : words) {
            double tf = 1. / words.size();
            word_to_document_freqs_[word][document_id] += tf;
        }

        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

private:

    map<string, map<int, double>> word_to_document_freqs_;    // хранит: слово (string), номера документов (int), tf (double)
    double document_count_ = 0;
    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;

        for (const string& word : SplitIntoWords(text)) {

            if (!IsStopWord(word)) {
                words.push_back(word);
            }

        }

        return words;
    }

    /* Отдельный метод определения: является ли слово "минус" или "плюс". Запись в структуру QueryWord. */
    QueryWord PareQueryWord(const string& text) const {
        QueryWord query_word;

        if (text[0] == '-') {
            query_word.is_minus = true;
            query_word.is_plus = false;
            query_word.word = text.substr(1);
        }
        else {
            query_word.is_plus = true;
            query_word.is_minus = false;
            query_word.word = text;
        }

        return query_word;
    }

    Query ParseQuery(const string& text) const {
        Query query;

        for (const string& word : SplitIntoWords(text)) {

            QueryWord temp_query_word = PareQueryWord(word);

            if (temp_query_word.is_minus) {
                query.minus_words.insert(temp_query_word.word);
            }

            if (!IsStopWord(temp_query_word.word)) {
                query.plus_words.insert(temp_query_word.word);
            }

        }

        return query;
    }

    /* Отдельный метод расчета IDF. */
    double CalculateIDF(const string& word, const double& tf) const {
        return log(document_count_ / static_cast<double>(word_to_document_freqs_.at(word).size())) * tf;
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> document_to_relevance;    // ключ id, значение - релевантность
        vector<Document> matched_documents;

        /* Перебераем плюс слова если содержаться в док-те, то для каждого id добавляем релевантность */
        for (const auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(word)) {
                for (auto& [id, tf] : word_to_document_freqs_.at(word)) {
                    document_to_relevance[id] += CalculateIDF(word, tf);
                }
            }
        }

        /* Удалем из document_to_relevance док-ты где есть минус слово */
        for (const auto& word : query.minus_words) {
            if (word_to_document_freqs_.count(word)) {
                for (auto& [id, tf] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(id);
                }
            }
        }

        /* Перемещаем результат в структуру */
        for (auto [id, relevance] : document_to_relevance) {
            matched_documents.push_back({ id, relevance });
        }

        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {

    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }

}