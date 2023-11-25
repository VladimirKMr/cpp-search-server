#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words) : SearchServer(SplitIntoWords(stop_words)) {}

SearchServer::SearchServer() = default;

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("invalid id");
    }
    if (documents_.count(document_id) != 0) {
        throw std::invalid_argument("id is busy");
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("words contain special characters");
    }

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();  // Для расчета TF

    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;  // Добавление TF в word_to_document_freqs_
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    id_number_.push_back(document_id);
}

// Поиск для определенного статуса
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus raw_status) const {
        return FindTopDocuments(raw_query, [raw_status]([[maybe_unused]] int document_id, DocumentStatus status, int rating [[maybe_unused]] )
            { return status == raw_status; } );
}

// Поиск актуальных документов (запрос с одним параметром)
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

// метод получения количества документов
int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

// метод получения id документа по индексу в словаре
int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index > id_number_.size()) {
        throw std::out_of_range("out of range documents");
    }
    return id_number_[index];
}

// Отдельный метод поиска слов в определенном документе. Возвращает кортеж (совпадающие слова, статус документа).
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    if (document_id < 0) {
        throw std::invalid_argument("invalid document_id");
    }

    std::vector<std::string> words_to_result;
    const Query query = ParseQuery(raw_query);

    for (const std::string& plus_word : query.plus_words) {
        if (word_to_document_freqs_.at(plus_word).count(document_id)) {
            words_to_result.push_back(plus_word);
        }
    }

    for (const std::string& minus_word : query.minus_words) {
        if (word_to_document_freqs_.at(minus_word).count(document_id)) {
            words_to_result.clear();
            break;
        }
    }
    auto match_document = std::tuple{ words_to_result, documents_.at(document_id).status };
    return match_document;
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

// статический метод проверки, что в слове нет спец символов с кодами от 0 до пробела
bool SearchServer::IsValidWord(const std::string& word) {
// проверяем на валидность по спецсимволам
    for (int i = 0; i < word.size(); ++i) {
        if (word[i] >= '\0' && word[i] < ' ') {
            return false;
        }
    }
    return true;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

// Статический метод расчета среднего рейтинга
int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

/* Отдельный метод определения: является ли слово "минус" или "плюс", "стоп-словом". Запись в структуру QueryWord. */
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Проверки на корректность запроса
    if (text[0] == '-') {
        if (text[0] == '-' && text[1] == '-') {
            throw std::invalid_argument("invalid query (double minus)");
        }
        if (text.size() == 1) {
            throw std::invalid_argument("invalid query (minus without word)");
        }
        is_minus = true;
        text = text.substr(1);
    }

    if (text[text.size() - 1] == '-') {
        throw std::invalid_argument("invalid query (minus end word)");
    }

    if (text.empty()) {
        throw std::invalid_argument("query word not found");
    }

    if (!IsValidWord(text)) {
        throw std::invalid_argument("query word with special characters");
    }
        
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required, IDF
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}