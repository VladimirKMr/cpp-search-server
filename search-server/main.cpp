#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
    // Параметризованный конструктор
    Document(int id, double relevance, int rating)
        : id(id), relevance(relevance), rating(rating) {
    }
    // Конструктор по умолчанию (с заданными значениями по умолчанию)
    Document() = default;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    template <typename StringContainer>  // шаблонный конструктор для контейнеров set и vector
    explicit SearchServer(const StringContainer& stop_words) {
        for (const string& word : stop_words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("invalid stop words"s);
            }
            stop_words_.insert(word);
        }
    }

    explicit SearchServer(const string& stop_words) {  // конструктор для стоп слов в виде строки
        if (!IsValidWord(stop_words)) {
            throw invalid_argument("invalid stop words"s);
        }
        for (const string& word : SplitIntoWords(stop_words)) {
            stop_words_.insert(word);
        }
    }

    SearchServer() = default; // Конструктор по умолчанию

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("invalid id"s);
        }
        if (documents_.count(document_id) != 0) {
            throw invalid_argument("id is busy"s);
        }
        if (!IsValidWord(document)) {
            throw invalid_argument("words contain special characters"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size(); // Для расчета TF

        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count; // Добавление TF в word_to_document_freqs_
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    // Поиск с фильтром (предикатом)
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate predicate) const {
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("invalid query"s);
        }

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
                    ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    // Поиск для определенного статуса
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus raw_status) const {
        return FindTopDocuments(raw_query, [raw_status]([[maybe_unused]] int document_id, DocumentStatus status, int rating [[maybe_unused]] )
            { return status == raw_status; } );
    }

    // Поиск актуальных документов (запрос с одним параметром)
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    // метод получения количества документов
    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    };

    // метод получения id документа по индексу в словаре
    int GetDocumentId(int index) const {
        if (index < 0 || index > GetDocumentCount() - 1) {
            throw out_of_range("out of range documents"s);
        }
        auto it = documents_.begin();
        advance(it, index);  // применил такой подход, чтобы не вводить новый словарь
        return it->first;
    }

    // Отдельный метод поиска слов в определенном документе. Возвращает кортеж (совпадающие слова, статус документа).
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (!IsValidWord(raw_query) || document_id < 0) {
            throw invalid_argument("invalid query"s);
        }

        vector<string> words_to_result;
        const Query query = ParseQuery(raw_query);

        for (const string& plus_word : query.plus_words) {
            if (word_to_document_freqs_.at(plus_word).count(document_id)) {
                words_to_result.push_back(plus_word);
            }
        }

        for (const string& minus_word : query.minus_words) {
            if (word_to_document_freqs_.at(minus_word).count(document_id)) {
                words_to_result.clear();
                break;
            }
        }
        auto match_document = tuple{ words_to_result, documents_.at(document_id).status };
        return match_document;
    };

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;  // хранит: слово (string), номера документов (int), tf (double)
    map<int, DocumentData> documents_; // id, rating, status

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    // статический метод проверки, что в слове нет спец символов с кодами от 0 до пробела
    static bool IsValidWord(const string& word) {
        // проверяем на валидность по различным требованиям (наличие спецсимволов, двойного тире, слово из одного тире, тире в конце слова)
        for (int i = 0; i < word.size(); ++i) {
            if (word[i] >= '\0' && word[i] < ' ') {
                return false;
            }
            if (word[i] == '-' && word[i + 1] == '-') {
                return false;
            }
            if (word[i] == '-' && i + 1 == word.size()) {
                return false;
            }
            if (word[i] == '-' && word.size() == 1) {
                return false;
            }
        }
        return true;
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

    // Статический метод расчета среднего рейтинга
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    /* Отдельный метод определения: является ли слово "минус" или "плюс", "стоп-словом". Запись в структуру QueryWord. */
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);

            if (IsValidWord(query_word.data)) {
                if (!query_word.is_stop) {
                    if (query_word.is_minus) {
                        query.minus_words.insert(query_word.data);
                    }
                    else {
                        query.plus_words.insert(query_word.data);
                    }
                }
            }
            else {
                query.minus_words.clear();
                query.plus_words.clear();
            }
        }
        return query;
    }

    // Existence required, IDF
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate predicate) const {
        map<int, double> document_to_relevance;  // ключ id, значение - релевантность

        /* Перебераем плюс слова если содержаться в док-те, то для каждого id добавляем релевантность */
        for (const string& word : query.plus_words) {
            if (!word_to_document_freqs_.count(word)) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                // документы добавляются в document_to_relevance только с условием предиката (фильтра)
                if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        /* Удалем из document_to_relevance док-ты где есть минус слово */
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        /* Перемещаем результат в структуру */
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

int main() {

    
    return 0;
}