#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "cachelab.h"

const char *options = "h v s: E: b: t:";
const char *help = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\
Options:\
  -h         Print this help message.\
  -v         Optional verbose flag.\
  -s <num>   Number of set index bits.\
  -E <num>   Number of lines per set.\
  -b <num>   Number of block offset bits.\
  -t <file>  Trace file.\
\
Examples:\
  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\
  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";

typedef struct cacheLineNode
{
    int tag;
    struct cacheLineNode *next;
} CacheLineNode;

typedef struct set
{
    int currentSize;
    int maxSize;
    CacheLineNode *head;
    CacheLineNode *tail;
} Set;

int hits;
int misses;
int evictions;
int blockOffsetMask;
int setNumberMask;
int dCacheSetBits;
int dCacheAssociativity;
int dCacheBlockBits;
Set *dCacheSets;
int dCacheVerboseMode;

int isFull(const Set *set)
{
    return set->currentSize == set->maxSize;
}

void evict(Set *set)
{
    CacheLineNode *evicted = set->head;
    if (set->head->next == NULL)
    {
        set->head = NULL;
        set->tail = NULL;
    }
    else
    {
        set->head = set->head->next;
    }

    set->currentSize--;

    free(evicted);

    evictions++;
    if (dCacheVerboseMode)
        fprintf(stdout, "eviction ");
}

void load(Set *set, int tag)
{
    if (isFull(set))
        evict(set);

    CacheLineNode *newLine = (CacheLineNode *)malloc(sizeof(CacheLineNode));
    newLine->tag = tag;
    newLine->next = NULL;

    if (set->tail == NULL)
    {
        set->head = newLine;
        set->tail = newLine;
    }
    else
    {
        set->tail->next = newLine;
        set->tail = newLine;
    }

    set->currentSize++;
}

int isCached(Set *set, int tag)
{
    int hit = 0;
    CacheLineNode *prev = NULL;
    CacheLineNode *current = set->head;
    while (current != NULL)
    {
        if (current->tag == tag)
        {
            hit = 1;
            if (current->next == NULL)
            {
                break;
            }

            if (prev != NULL)
            {
                prev->next = current->next;
            }
            else
            {
                set->head = current->next;
            }

            set->tail->next = current;
            set->tail = current;
            current->next = NULL;
            break;
        }

        prev = current;
        current = current->next;
    }

    if (hit)
    {
        hits++;
        if (dCacheVerboseMode)
            fprintf(stdout, "hit ");
    }
    else
    {
        misses++;
        if (dCacheVerboseMode)
            fprintf(stdout, "miss ");
    }

    return hit;
}

int createRightBitMask(int numberOfBits)
{
    int mask = 0;
    while (numberOfBits)
    {
        mask ^= 1 << --numberOfBits;
    }

    return mask;
}

void initializeDCache(int setBits, int associativity, int blockBits, int verboseMode)
{
    hits = 0;
    misses = 0;
    evictions = 0;
    dCacheSetBits = setBits;
    dCacheAssociativity = associativity;
    dCacheBlockBits = blockBits;
    dCacheVerboseMode = verboseMode > 0 ? verboseMode : 0;
    setNumberMask = createRightBitMask(dCacheSetBits);
    blockOffsetMask = createRightBitMask(dCacheBlockBits);

    int setNumber = 1 << dCacheSetBits;
    dCacheSets = (Set *)malloc(setNumber * sizeof(Set));
    for (int i = 0; i < setNumber; i++)
    {
        dCacheSets[i].currentSize = 0;
        dCacheSets[i].maxSize = dCacheAssociativity;
        dCacheSets[i].head = NULL;
        dCacheSets[i].tail = NULL;
    }
}

void parseAddress(unsigned int address, int *setNumber, int *tag, int *blockOffset)
{
    *blockOffset = address & blockOffsetMask;
    unsigned int cutOffset = address >> dCacheBlockBits;
    *setNumber = cutOffset & setNumberMask;
    *tag = cutOffset >> dCacheSetBits;
}

void dCacheOperate(Set *set, int tag)
{
    if (!isCached(set, tag))
    {
        load(set, tag);
    }
}

int dCacheSimulate(char command, int address, int bytes)
{
    if (dCacheVerboseMode)
    {
        fprintf(stdout, "%c %x,%d ", command, address, bytes);
    }

    int setNumber;
    int tag;
    int offsetNumber;
    parseAddress(address, &setNumber, &tag, &offsetNumber);
    Set *set = &dCacheSets[setNumber];
    switch (command)
    {
    case 'S':
    case 'L':
    {
        dCacheOperate(set, tag);
        break;
    }
    case 'M':
    {
        dCacheOperate(set, tag);
        dCacheOperate(set, tag);
        break;
    }
    default:
        return -1;
    }

    if (dCacheVerboseMode)
        fprintf(stdout, "\n");

    return 0;
}

int parseIntegerOption(char option, const char *optionValue, int *value)
{
    if (sscanf(optionValue, "%d", value) == 0)
    {
        fprintf(stderr,
                "Invalid value %s for option -%c, integer required.\n",
                optionValue,
                option);

        return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
    opterr = 0;
    int optionVerboseMode = 0;
    int optionSetBits = 0;
    int optionAssociativity = 0;
    int optionBlockBits = 0;
    char *optionTraceFile = NULL;
    int c;
    while ((c = getopt(argc, argv, options)) != -1)
        switch (c)
        {
        case 'h':
            fprintf(stderr, "%s", help);
            return 0;
        case 'v':
            optionVerboseMode = 1;
            break;
        case 's':
            if (!parseIntegerOption(c, optarg, &optionSetBits))
                return -1;
            break;
        case 'E':
            if (!parseIntegerOption(c, optarg, &optionAssociativity))
                return -1;
            break;
        case 'b':
            if (!parseIntegerOption(c, optarg, &optionBlockBits))
                return -1;
            break;
        case 't':
            optionTraceFile = optarg;
            break;
        case '?':
            if (optopt == 's' ||
                optopt == 'E' ||
                optopt == 'b' ||
                optopt == 't')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option -%c.\n", optopt);
            else
                fprintf(stderr, "Unknown option character \\x%x.\n", optopt);
            return -1;
        default:
            abort();
        }

    if (optionSetBits == 0 ||
        optionAssociativity == 0 ||
        optionBlockBits == 0 ||
        optionTraceFile == NULL)
    {
        fprintf(stderr, "Missing required argument...");
        fprintf(stderr, "%s", help);
    }

    initializeDCache(optionSetBits, optionAssociativity, optionBlockBits, optionVerboseMode);

    FILE *fp = fopen(optionTraceFile, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "No such file or directory - %s", optionTraceFile);
        return -1;
    }

    size_t max_length = 32;
    char *line = (char *)malloc(max_length * sizeof(char));
    while (getline(&line, &max_length, fp) != -1)
    {
        if (!isspace(line[0]))
            continue;

        char parsedCommand;
        int parsedAddress;
        int parsedBytes;
        sscanf(line, " %c %x,%d", &parsedCommand, &parsedAddress, &parsedBytes);
        dCacheSimulate(parsedCommand, parsedAddress, parsedBytes);
    }

    fclose(fp);

    printSummary(hits, misses, evictions);
    return 0;
}