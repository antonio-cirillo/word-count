#include "sort.h"

void swap(Word *w1, Word *w2) {

    Word t = *w1;
    *w1 = *w2;
    *w2 = t;

}

int partition(Word words[], int low, int high) {

    Word pivot = words[high];
    int i = (low - 1);

    for (int j = low; j < high; j++) { 

        if (words[j].occurrences >= pivot.occurrences) {

            i++;
            swap(&words[i], &words[j]);

        }

    }

    swap(&words[i + 1], &words[high]);

    return (i + 1);

}

void quick_sort(Word words[], int low, int high) {
    
    if (low < high) {

        int pi = partition(words, low, high);
        quick_sort(words, low, pi - 1);
        quick_sort(words, pi + 1, high);

    }

}