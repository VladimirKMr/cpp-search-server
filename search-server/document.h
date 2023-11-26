#pragma once
#include <iostream>
#include <vector>

struct Document {
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
    Document(int id, double relevance, int rating);
    Document();
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

namespace std {
void PrintDocument(const Document& document);

void PrintDocument(const vector<Document>& documents);

ostream& operator<<(ostream& out, const Document& doc);

ostream& operator<<(ostream& out, const vector<Document>& vec);
}