#include <string.h>

#include "sort.h"

void swap(Word *w1, Word *w2) {

    Word t = *w1;
    *w1 = *w2;
    *w2 = t;

}

int compare_occurrences(Word w1, Word w2) {

    if (w1.occurrences == w2.occurrences)
        return strcmp(w1.lexeme, w2.lexeme) < 0;
    else 
        return w1.occurrences >= w2.occurrences;

}

int compare_lexeme(Word w1, Word w2) {
    return strcmp(w1.lexeme, w2.lexeme) < 0;
}

int partition(Word words[], int low, int high, int (*f)(Word, Word)) {

    Word pivot = words[high];
    int i = (low - 1);

    for (int j = low; j < high; j++) { 

        if (f(words[j], pivot)) {

            i++;
            swap(&words[i], &words[j]);

        }

    }

    swap(&words[i + 1], &words[high]);

    return (i + 1);

}

void quick_sort(Word words[], int low, int high, int (*f)(Word, Word)) {
    
    if (low < high) {

        int pi = partition(words, low, high, f);
        quick_sort(words, low, pi - 1, f);
        quick_sort(words, pi + 1, high, f);

    }

}