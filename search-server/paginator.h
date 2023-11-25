#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include "document.h"

template <typename PaginatorIterator>
class IteratorRange {
public:
    IteratorRange (PaginatorIterator begin, PaginatorIterator end, size_t page_size) 
        : begin_(begin), end_(end), page_size_(page_size) {

        }
    
    auto Begin() const {
        return begin_;
    }
    auto End() const {
        return end_;
    }
    auto size() const {
        return page_size_;
    }
 
private:
    PaginatorIterator begin_;
    PaginatorIterator end_;
    size_t page_size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator (Iterator begin, Iterator end, size_t page_size) {
        if (page_size == 0) {
            throw std::invalid_argument("Incorrect page size");
        }

        for (size_t i = distance(begin, end); i > 0; ) {
            auto size = std::min(i, page_size);
            auto page_end = begin + size;
            pages_.push_back({begin, page_end, size});
            begin = page_end;
            i -= size;
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator (begin(c), end(c), page_size);
}

// ========== перегрузки для вывода страниц поиска ==========

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

}

// Перегрузка для вывода IteratorRange (документов в одной странице - класса IteratorRange)
template <typename Iterator>
std::ostream& std::operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (auto it = range.Begin(); it != range.End(); ++it) {
        out << *it;
    }
    return out;
}

// Перегрузка для вывода Paginator (всех страниц - вектора страниц в классе Paginator)
template <typename Iterator>
std::ostream& std::operator<<(std::ostream& out, const Paginator<Iterator>& paginator) {
    for (const auto& page : paginator) {
        out << page;
    }
    return out;
}