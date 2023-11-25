#include <string>
#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {  
}

void RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus raw_status) {
    return AddFindRequest(raw_query, [raw_status]([[maybe_unused]] int document_id, DocumentStatus status, int rating [[maybe_unused]] )
        { return status == raw_status; } );
}

void RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_;
}