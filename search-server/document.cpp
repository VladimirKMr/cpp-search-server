#include "document.h"

Document::Document (int id, double relevance, int rating) 
        : id(id), relevance(relevance), rating(rating) {
}

Document::Document() = default;

namespace std {
ostream& operator<<(ostream& out, const Document& doc) {
    out << "{ document_id = "s << doc.id << ", relevance = "s 
    << doc.relevance << ", rating = "s << doc.rating << " }"s;
    return out;
}

ostream& operator<<(ostream& out, const vector<Document>& vec) {
    for (const auto& doc : vec) {
        out << doc << endl;
    }
    return out;
}

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void PrintDocument(const vector<Document>& documents) {
    for (const Document& doc : documents) {
        cout << doc << endl;
    }
}
}