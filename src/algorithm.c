#include "algorithm.h"

char* find_substr(char* const begin, char* const end, char* const substr)
{
    char* position = begin;
    while (position < end) {
        char* i = position;
        char* j = substr;
        while (i < end && *i == *j) {
            ++i;
            ++j;
        }
        if (i < end && *j == '\0') {
            return position;
        }

        ++position;
    }

    return end;
}

void backward_copy(const char* begin, const char* end, char* to)
{
    while (begin < end) {
        *to = *begin;
        ++to;
        ++begin;
    }
}

void swap_char(char* begin, char* end, char change, char change_to)
{
    while (begin < end) {
        if (*begin == change) {
            *begin = change_to;
        }

        ++begin;
    }
}