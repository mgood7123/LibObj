//License MIT 2017 Ahmad Retha

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include "UnrestrictedMultiShiftAnd.h"

#if INTPTR_MAX == INT32_MAX
    #define ffs(x) __builtin_ffs((x))
#elif INTPTR_MAX == INT64_MAX
    #define ffs(x) __builtin_ffsl((x))
#else
    #error "Unsupported architecture - neither 64 or 32 bit!"
#endif

using namespace std;

/**
 * @constructor An alphabet must be supplied of the letters that will be used in the patterns
 *
 * @param alphabet A list of characters used in the text/patterns e.g. ACGT
 * @param reportPatternPositions Report positions when matches found?
 */
UnrestrictedMultiShiftAnd::UnrestrictedMultiShiftAnd(const string & alphabet, bool reportPatternPositions)
{
    this->alphabet = alphabet;
    this->reportPatterns = reportPatternPositions;
    this->N = 0;
    this->M = 0;
    this->M_Max = 0;
    this->L = 1;
    this->Sv.push_back(0ul);
    this->Ev.push_back(0ul);
    this->D.push_back(0ul);
    vector<WORD> v;
    unsigned int i;
    for (i = 0; i < this->alphabet.length(); i++)
    {
        this->Sigma[(int)alphabet[i]] = i + 1;
        v.push_back(0ul);
    }
    this->Bv.push_back(v);
}

/**
 * Add a pattern
 *
 * @param pattern
 */
void UnrestrictedMultiShiftAnd::addPattern(const string & pattern)
{
    if (alphabet.length() == 0) {
        return;
    }
    if (pattern.length() == 0) {
        return;
    }
    int charIdx;
    unsigned int wordIdx, bitIdx, i, j, m = pattern.length();
    unsigned int lastIdx = m - 1;

    for (i = 0; i < m; i++)
    {
        charIdx = (int) this->Sigma[(int)pattern[i]];
        wordIdx = (unsigned int) ((double)(this->M + i) / (double)BITSINWORD);
        bitIdx = (unsigned int) ((this->M + i) % BITSINWORD);

        //expand memory if required
        if (this->L == wordIdx)
        {
            this->Sv.push_back(0ul);
            this->Ev.push_back(0ul);
            this->D.push_back(0ul);
            vector<WORD> v;
            for (j = 0; j < this->alphabet.length(); j++)
            {
                v.push_back(0ul);
            }
            this->Bv.push_back(v);
            this->L = this->L + 1;
        }

        //mark letter position of pattern
        if (charIdx > 0) {
            this->Bv[wordIdx][charIdx - 1] = this->Bv[wordIdx][charIdx - 1] | (1ul << bitIdx);
        }
        //mark start position of pattern
        if (i == 0) {
            this->Sv[wordIdx] = this->Sv[wordIdx] | (1ul << bitIdx);
        }
        //mark end position of pattern
        if (i == lastIdx) {
            this->Ev[wordIdx] = this->Ev[wordIdx] | (1ul << bitIdx);
        }
    }

    this->M = this->M + m;
    this->M_Max = std::max(this->M_Max, m);

    //keep a record of position the pattern is stored and pattern id - for search results
    if (this->reportPatterns) {
        this->positions.insert(pair<int, pair<int, int>>(this->M - 1, pair<int, int>(this->N, m == 0 ? 0 : m-1)));
    }

    this->N = this->N + 1;
}

bool UnrestrictedMultiShiftAnd::search(const string & text)
{
    return this->search(text, 0);
}

bool UnrestrictedMultiShiftAnd::search(const string & text, unsigned int pos)
{
    return this->search(text.c_str(), pos, text.length());
}

bool UnrestrictedMultiShiftAnd::search(const char * text, unsigned int pos, unsigned int len)
{
    if (len == 0) {
        return false;
    }
    //initialize vector D to have a fresh state for the new search
    this->D.assign(this->L, 0ul);
    return this->search(text, this->D, pos, len);
}

bool UnrestrictedMultiShiftAnd::search(const string & text, vector<WORD> & startingSearchState)
{
    return this->search(text.c_str(), startingSearchState, 0, text.length());
}

bool UnrestrictedMultiShiftAnd::search(const string & text, vector<WORD> & startingSearchState, unsigned int pos)
{
    return this->search(text.c_str(), startingSearchState, pos, text.length());
}

bool UnrestrictedMultiShiftAnd::search(const char * text, vector<WORD> & startingSearchState, unsigned int pos, unsigned int len)
{
    if (len == 0) {
        return false;
    }

    unsigned int i, j, k, n;

    if (this->M == 0) {
        cerr << "No patterns added to search." << endl;
        return false;
    }

    //Make sure vector D has sufficient memory for the search
    this->D = startingSearchState;
    if (this->D.size() < this->L) {
        j = this->L - this->D.size();
        for (i = 0; i < j; i++) {
            this->D.push_back(0ul);
        }
    }

    //calculate starting and ending positions of search
    i = min((int)pos, (int)len - 1);
    n = min(i + len, (unsigned int)len);

    //init tracking vars
    int charIdx;
    bool zeroed = false;
    bool matchFound = false;
    WORD temp, carry, checking;
    WORD carryMask = 1ul << (BITSINWORD - 1);

    //loop through the text
    for (; i < n; i++)
    {
        carry = 0;
        charIdx = (int) this->Sigma[(int)text[i]] - 1;

        if (charIdx == -1)
        {
            if (!zeroed)
            {
                for (j = 0; j < this->L; j++)
                {
                    this->D[j] = 0;
                }
                zeroed = true;
            }
        }
        else
        {
            //loop through the words
            for (j = 0; j < this->L; j++)
            {
                temp = this->D[j];

                this->D[j] = (((this->D[j] << 1) | carry) | this->Sv[j]) & this->Bv[j][charIdx];

                //check if any matches found
                checking = this->D[j] & this->Ev[j];
                if (checking)
                {
                    matchFound = true;

                    //find out position(s) of the match
                    while (checking)
                    {
                        k = ffs(checking) - 1;
                        if (this->reportPatterns) {
                            this->matches.insert(pair<int,pair<int, int>>((int)i, this->positions.find(j * BITSINWORD + k)->second));
                        } else {
                            this->matches.insert(pair<int,pair<int, int>>((int)i, pair<int, int>(-1, 0)));
                        }
                        checking = checking ^ (1ul << k);
                    }
                }

                carry = (WORD) ((carryMask & temp) != 0);
            }
            zeroed = false;
        }
    }

    return matchFound;
}

vector<WORD> UnrestrictedMultiShiftAnd::getLastSearchState() const
{
    return this->D;
}

void UnrestrictedMultiShiftAnd::clearMatches()
{
    this->matches.clear();
}

unsigned int UnrestrictedMultiShiftAnd::getNumberOfPatterns() const
{
    return this->N;
}

unsigned int UnrestrictedMultiShiftAnd::getTotalPatternLength() const
{
    return this->M;
}

unsigned int UnrestrictedMultiShiftAnd::getMaxPatternLength() const
{
    return this->M_Max;
}

multimap<int,std::pair<int, int>> UnrestrictedMultiShiftAnd::getMatches() const
{
    return this->matches;
}
