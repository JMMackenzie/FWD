create_docvectors:
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG -o obj/create_docvectors.cpp.o -c create_docvectors.cpp
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG  -o obj/compress_qmx.cpp.o -c compress_qmx.cpp
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG  -rdynamic obj/create_docvectors.cpp.o obj/compress_qmx.cpp.o -o create_docvectors

query_docvectors:
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG -o obj/query_docvectors.cpp.o -c query_docvectors.cpp
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG  -o obj/compress_qmx.cpp.o -c compress_qmx.cpp
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG  -rdynamic obj/query_docvectors.cpp.o obj/compress_qmx.cpp.o -o query_docvectors
#	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -g -o obj/query_docvectors.cpp.o -c query_docvectors.cpp
#	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -g -rdynamic obj/query_docvectors.cpp.o obj/compress_qmx.cpp.o -o d_query_docvectors



dump_to_plain:
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG -o obj/dump_to_plain.cpp.o -c dump_to_plain.cpp
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG  -o obj/compress_qmx.cpp.o -c compress_qmx.cpp
	g++ -std=c++17 -march=native -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-local-typedefs -O3 -DNDEBUG  -rdynamic obj/dump_to_plain.cpp.o obj/compress_qmx.cpp.o -o dump_to_plain


clean:
	rm dump_to_plain query_docvectors create_docvectors
