#pragma once

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