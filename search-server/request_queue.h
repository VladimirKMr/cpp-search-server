#pragma once

#include <deque>
#include <string>
#include <vector>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    void AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    void AddFindRequest(const std::string& raw_query, DocumentStatus raw_status);

    void AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string raw_query;
        std::vector<Document> result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int no_result_requests_ = 0;
    int result_requests_ = 0;
    const SearchServer& search_server_;
};

template <typename DocumentPredicate>
void RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
        QueryResult temp;
        
        if (result.empty()) {
            temp.raw_query = raw_query;
            temp.result = result;
            ++no_result_requests_;
            requests_.push_back(temp);
        }
        else {
            temp.raw_query = raw_query;
            temp.result = result;
            ++result_requests_;
            requests_.push_back(temp);
        }
        if (no_result_requests_ + result_requests_ >= min_in_day_) {
            requests_.pop_front();
            --no_result_requests_;
            --result_requests_;
        }
}