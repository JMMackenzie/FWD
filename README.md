# >> FWD >>

This is a prototype forward index-based system for simple and efficient re-ranking.

Documentation to follow, but in short:

1. `create_docvectors` will build a forward index; requires PISA canonical input files.
2. `query_docvectors` allows you to pass an index, a query file, and a TREC run; it re-ranks the TREC run according to the dot-product of the matching terms in the given query.

## Credits

- QMX Compression: Part of the [JASS](https://github.com/lintool/JASS) search system.
- This project stemmed from [RMQV](https://github.com/JMMackenzie/RMQV/) ([or here](https://github.com/rmit-ir/RMQV)) and was initially used to generate statistics for RM query expansion.
- Recent related work: [fast-forward-indexes](https://github.com/mrjleo/fast-forward-indexes)


