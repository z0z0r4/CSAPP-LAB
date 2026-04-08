#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define hash_size 1048576
#define word_length 100

struct Node
{
    char value[word_length];
    int count;
    struct Node *next;
};

struct HashMap
{
    struct Node **buckets;
    int size;
};

int get_hash(const char *key, int size)
{
    int hash = 0;
    while (*key)
    {
        hash = (hash * 31 + *key++) & (size - 1);
    }
    return hash;
}

void toLowerCase(char *str)
{
    while (*str)
    {
        if (*str >= 'A' && *str <= 'Z')
        {
            *str += 32;
        }
        str++;
    }
}

void init_hash_map(struct HashMap *map, const int size)
{
    map->size = size;
    map->buckets = (struct Node **)malloc(map->size * sizeof(struct Node *));

    for (int i = 0; i < map->size; ++i)
    {
        map->buckets[i] = NULL;
    }
}

void insert(struct HashMap *map, const char *key, const int hash)
{
    struct Node *curr = map->buckets[hash];
    while (curr != NULL)
    {
        if (strcmp(curr->value, key) == 0)
        {
            curr->count++; // word already exists, increment count
            return;
        }
        curr = curr->next;
    }

    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    strncpy(node->value, key, word_length);
    node->count = 1;
    node->next = map->buckets[hash];
    map->buckets[hash] = node; // head insert
}

struct HashMap *init_words(char *words[], int num_words)
{
    struct HashMap *map = (struct HashMap *)malloc(sizeof(struct HashMap));
    init_hash_map(map, hash_size);

    for (int i = 0; i < num_words; ++i)
    {
        int hash = get_hash(words[i], hash_size);
        insert(map, words[i], hash);
    }
    return map;
}

void stat_count(struct HashMap *word_map, struct HashMap *count_map, const int num_words)
{
    for (int i = 0; i < word_map->size; ++i)
    {
        struct Node *node = word_map->buckets[i];
        while (node != NULL)
        {
            {
                struct Node *curr_count_node = (struct Node *)malloc(sizeof(struct Node));
                strcpy(curr_count_node->value, node->value);
                curr_count_node->count = -1;
                curr_count_node->next = count_map->buckets[node->count];
                count_map->buckets[node->count] = curr_count_node;
            }
            node = node->next;
        }
    }
}

void print_stat(struct HashMap *map)
{
    for (int i = 0; i < map->size; ++i)
    {
        struct Node *node = map->buckets[i];
        while (node != NULL)
        {
            printf("%d: %s\n", i, node->value);
            node = node->next;
        }
    }
}

void free_hash_map(struct HashMap *map)
{
    for (int i = 0; i < map->size; ++i)
    {
        struct Node *node = map->buckets[i];
        while (node != NULL)
        {
            struct Node *temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(map->buckets);
    free(map);
}

void free_words(char **words, int num_words)
{
    for (int i = 0; i < num_words; ++i)
    {
        free(words[i]);
    }
    free(words);
}

void clean_word(char *str)
{
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++)
    {
        // 只保留字母和数字
        if (isalnum((unsigned char)str[i]))
        {
            str[j++] = tolower((unsigned char)str[i]); // 顺便转成小写，方便统一统计
        }
    }
    str[j] = '\0'; // 重新放置结束符
}

char **readFile(const char *filename, int *num_words)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char **words = NULL;
    char buffer[word_length];
    *num_words = 0;

    while (fscanf(file, "%19s", buffer) == 1)
    {
        clean_word(buffer);

        if (strlen(buffer) > 0)
        {
            words = (char **)realloc(words, (*num_words + 1) * sizeof(char *));
            words[*num_words] = strdup(buffer);
            (*num_words)++;
        }
    }

    fclose(file);
    return words;
}

char **generate_n_grams(char *words[], int num_words, int n, int *num_ngrams)
{
    char **ngrams = NULL;
    int buffer_size = n * (word_length + 1);
    char *buffer = (char *)malloc(buffer_size);
    *num_ngrams = 0;

    for (int i = 0; i <= num_words - n; ++i)
    {
        buffer[0] = '\0';
        for (int j = 0; j < n; ++j)
        {
            
            strcat(buffer, words[i + j]);
            if (j < n - 1)
                strcat(buffer, " ");
        }
        ngrams = (char **)realloc(ngrams, (*num_ngrams + 1) * sizeof(char *));
        ngrams[*num_ngrams] = strdup(buffer);
        (*num_ngrams)++;
    }
    free(buffer);
    return ngrams;
}

int main()
{
    char **raw_words;
    char **n_grams;
    int num_words;
    int num_ngrams;
    raw_words = readFile("input.txt", &num_words);
    n_grams = generate_n_grams(raw_words, num_words, 2, &num_ngrams);
    struct HashMap *map = init_words(n_grams, num_ngrams);

    struct HashMap *count_result = (struct HashMap *)malloc(sizeof(struct HashMap));
    init_hash_map(count_result, num_ngrams);

    stat_count(map, count_result, num_ngrams);
    // print_stat(count_result);

    free_hash_map(map);
    free_hash_map(count_result);
    free_words(raw_words, num_words);
    free_words(n_grams, num_ngrams);
    return 0;
}