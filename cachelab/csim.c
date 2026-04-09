#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char *file_name;
int s, E, b, group_cnt, trace_count;
int hit_count = 0, miss_count = 0, eviction_count = 0;
struct cacheGroup *cache;

struct trace
{
    char operation;
    int address;
    int size;
};

struct cacheRow
{
    int valid;
    long long tag;
    int last_time;
};

struct cacheGroup
{
    struct cacheRow *lines;
};

void parseAddress(int address, int *group_index, long long *tag, int *block_offset)
{
    *group_index = (address >> b) & ((1 << s) - 1);
    *tag = (long long)(address >> (b + s));
    *block_offset = address & ((1 << b) - 1);
}

struct trace *readTraceFile()
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", file_name);
        exit(1);
    }
    char operation;
    int address, size;
    int traces_size = 1000;
    struct trace *traces = malloc(sizeof(struct trace) * traces_size);
    trace_count = 0;
    while (fscanf(file, " %c %x,%d", &operation, &address, &size) == 3)
    {
        struct trace t = {operation, address, size};
        *(traces + trace_count++) = t;
        if (trace_count >= traces_size)
        {
            traces_size *= 2;
            traces = realloc(traces, sizeof(struct trace) * traces_size);
        }
    }
    fclose(file);
    return traces;
}

int str2int(char *str)
{
    int result = 0;
    while (*str)
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

void parseArguments(int argc, char *argv[])
{
    for (int i = 1; i < argc; i += 2)
    {
        if (argv[i][1] == 't')
        {
            file_name = argv[i + 1];
        }
        else
        {
            int val = str2int(argv[i + 1]);
            switch (argv[i][1])
            {
            case 's':
                s = val;
                break;
            case 'E':
                E = val;
                break;
            case 'b':
                b = val;
                break;
            }
        }
    }
    group_cnt = 1 << s;
}

void touchCache(int group_index, long long tag)
{
    int empty_line_index = -1;
    for (int i = 0; i < E; i++)
    {
        if (cache[group_index].lines[i].valid && cache[group_index].lines[i].tag == tag)
        {
            // hit case
            cache[group_index].lines[i].last_time = clock();
            hit_count++;
            return;
        }
        else
        {
            if (!cache[group_index].lines[i].valid)
            {
                // record empty line
                empty_line_index = i;
            }
        }
    }

    miss_count++;

    // if group has empty line
    if (empty_line_index != -1)
    {
        cache[group_index].lines[empty_line_index].valid = 1;
        cache[group_index].lines[empty_line_index].tag = tag;
        cache[group_index].lines[empty_line_index].last_time = clock();
        return;
    }

    // if we reach here, where no empty line, it means we have a miss and need to evict
    eviction_count++;
    int lru_index = 0;
    for (int i = 1; i < E; i++)
    {
        if (cache[group_index].lines[i].last_time < cache[group_index].lines[lru_index].last_time)
        {
            lru_index = i;
        }
    }

    // evict the LRU line
    cache[group_index].lines[lru_index].tag = tag;
    cache[group_index].lines[lru_index].last_time = clock();
}

void simulateCache(struct trace *traces)
{
    cache = malloc(sizeof(struct cacheGroup) * group_cnt);
    for (int i = 0; i < group_cnt; i++)
    {
        cache[i].lines = malloc(sizeof(struct cacheRow) * E);
        for (int j = 0; j < E; j++)
        {
            cache[i].lines[j].valid = 0;
            cache[i].lines[j].tag = 0;
            cache[i].lines[j].last_time = 0;
        }
    }

    for (int i = 0; i < trace_count; i++)
    {
        int group_index, block_offset;
        long long tag;
        parseAddress(traces[i].address, &group_index, &tag, &block_offset);
        if (traces[i].operation == 'L' || traces[i].operation == 'S')
        {
            touchCache(group_index, tag);
        }
        else if (traces[i].operation == 'M')
        {
            touchCache(group_index, tag);
            touchCache(group_index, tag);
        }
    }
}

int main(int argc, char *argv[])
{
    parseArguments(argc, argv);
    struct trace *traces = readTraceFile();
    simulateCache(traces);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}