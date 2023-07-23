//License MIT 2017 Ahmad Retha

#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

#ifndef __UMSA__
#define __UMSA__

#define WORD unsigned long int
#define WORDSIZE sizeof(WORD)
#define BITSINWORD (WORDSIZE * 8)

class UnrestrictedMultiShiftAnd
{
protected:
    /**
     * @var L The number of computer words needed to store the patterns
     */
    unsigned int L;
    /**
     * @var M The total length of the patterns
     */
    unsigned int M;
    unsigned int M_Max;
    /**
     * @var n The number of patterns added
     */
    unsigned int N;
    /**
     * @var Sv The bitvector holding the initial starting positions (states) of patterns
     */
    std::vector<WORD> Sv;
    /**
     * @var Ev The bitvector holding the ending positions (states) of patterns
     */
    std::vector<WORD> Ev;
    /**
     * @var Bv The bitvector that holds the positions of a character for all
     * characters. The first vector is the WORD position and the second is a
     * vector of WORDS encoding characters and their positions.
     */
    std::vector<std::vector<WORD>> Bv;
    /**
     * @var D Delta, the vector holding the current state of the search
     */
    std::vector<WORD> D;
    /**
     * @var Sigma The indexes of letters using the ascii table of characters
     */
    unsigned char Sigma[256] = {0};
    /**
     * @var alphabet The alphabet to use
     */
    std::string alphabet;
    /**
     * @var matches A multimap of ending positions where a match was found and
     * the id of the pattern discovered, <index_in_t, <pattern_id, pattern_length>>
     */
    std::multimap<int,std::pair<int, int>> matches;
    /**
     * @var positions A map of ending positions and the id of the pattern that is found there and the pattern length
     */
    std::map<int,std::pair<int, int>> positions;
    /**
     * @var reportPatterns
     */
    bool reportPatterns;

public:
    UnrestrictedMultiShiftAnd(const std::string & alphabet, bool reportPatternPositions = true);
    void addPattern(const std::string & pattern);

    /**
    * Search a text
    *
    * @param text
    * @return True if one or matches found, otherwise False
    */
    bool search(const std::string & text);

    /**
    * Search a text starting at position i
    *
    * @param text
    * @param pos position in text to start searching from
    * @return True if one or more matches found, otherwise False
    */
    bool search(const std::string & text, unsigned int pos);

    /**
    * Search a text starting at position i
    *
    * @param text
    * @param pos position in text to start searching from
    * @param len The number of characters to search from position pos
    * @return True if one or more matches found, otherwise False
    */
    bool search(const char * text, unsigned int pos, unsigned int len);

    /**
    * Search a text but supply an initial search state - this is useful for searching
    * text provided intermittently (on-line algorithm).
    *
    * @param text
    * @param startingSearchState A vector<WORD> with L elements
    * @return True if one or more matches found, otherwise False
    */
    bool search(const std::string & text, std::vector<WORD> & startingSearchState);

    /**
    * Search a text but supply an initial search state - this is useful for searching
    * text provided intermittently (online algorithm). For simple searches just use
    * UnrestrictedMultiShiftAnd::search()
    *
    * @param text
    * @param startingSearchState A vector<WORD> with L elements
    * @param pos position in text to start searching from
    * @return True if one or matches found, otherwise False
    */
    bool search(const std::string & text, std::vector<WORD> & startingSearchState, unsigned int pos);

    /**
    * Search a text but supply an initial search state - this is useful for searching
    * text provided intermittently (online algorithm). For simple searches just use
    * UnrestrictedMultiShiftAnd::search()
    *
    * @param text
    * @param startingSearchState A vector<WORD> with L elements
    * @param pos position in text to start searching from
    * @param len The number of characters to search from position pos
    * @return True if one or matches found, otherwise False
    */
    bool search(const char * text, std::vector<WORD> & startingSearchState, unsigned int pos, unsigned int len);

    /**
    * Get a multimap of all the matches found - <index_in_t, pattern_id>
    */
    std::multimap<int,std::pair<int, int>> getMatches() const;

    /**
    * After doing a search, get the last state of the search
    */
    std::vector<WORD> getLastSearchState() const;

    /**
    * Clear matches
    */
    void clearMatches();

    /**
    * Get the number of patterns already added
    */
    unsigned int getNumberOfPatterns() const;

    /**
    * Get the total length of the patterns added
    */
    unsigned int getTotalPatternLength() const;

    /**
    * Get the maximum length of the patterns added
    */
    unsigned int getMaxPatternLength() const;
};

#endif
