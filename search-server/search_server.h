#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <numeric>
#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>  // шаблонный конструктор для контейнеров set и vector
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words);

    SearchServer();  // Конструктор по умолчанию

    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings);

    // Поиск с фильтром (предикатом)
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate predicate) const;

    // Поиск для определенного статуса
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus raw_status) const;

    // Поиск актуальных документов (запрос с одним параметром)
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    // метод получения количества документов
    int GetDocumentCount() const;

    // метод получения id документа по индексу в словаре
    int GetDocumentId(int index) const;

    // Отдельный метод поиска слов в определенном документе. Возвращает кортеж (совпадающие слова, статус документа).
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    std::vector<int> id_number_;
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;  // хранит: слово (string), номера документов (int), tf (double)
    std::map<int, DocumentData> documents_; // id, rating, status

    bool IsStopWord(const std::string& word) const;

    // статический метод проверки, что в слове нет спец символов с кодами от 0 до пробела
    static bool IsValidWord(const std::string& word);

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    // Статический метод расчета среднего рейтинга
    static int ComputeAverageRating(const std::vector<int>& ratings);

    /* Отдельный метод определения: является ли слово "минус" или "плюс", "стоп-словом". Запись в структуру QueryWord. */
    QueryWord ParseQueryWord(std::string text) const;

    Query ParseQuery(const std::string& text) const;

    // Existence required, IDF
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate predicate) const;


};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
    for (const std::string& word : stop_words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("invalid stop words");
        }
        stop_words_.insert(word);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate predicate) const {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate predicate) const {
    std::map<int, double> document_to_relevance;  // ключ id, значение - релевантность

    /* Перебераем плюс слова если содержаться в док-те, то для каждого id добавляем релевантность */
    for (const std::string& word : query.plus_words) {
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
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    /* Перемещаем результат в структуру */
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}
