/**
 * @file all.cpp
 *
 * @brief Gather all other files to facilitate compiler inter-procedural optimization.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

/* C++ and Boost headers */
#include "util/cpp_headers.h"

/* miscellaneous utilities */
#include "util/options.cpp"
#include "util/util.cpp"
#include "util/stats.cpp"
#include "core/bit.cpp"

/* move generation */
#include "core/board.cpp"
#include "core/move.cpp"

/* eval & search */
#include "eval/eval.cpp"
#include "core/hash.cpp"
#include "search/ybwc.cpp"
#include "search/search.cpp"
#include "search/endgame.cpp"
#include "search/midgame.cpp"
#include "search/root.cpp"

/* miscellaneous tests */
#include "app/perft.cpp"
#include "app/obftest.cpp"
#include "app/histogram.cpp"
#include "app/bench.cpp"

/* opening book & game database */
#include "book/book.cpp"
#include "game/game.cpp"
#include "game/base.cpp"
#include "game/opening.cpp"

/* game play with various protocols */
#include "protocol/play.cpp"
#include "protocol/event.cpp"
#include "protocol/ui.cpp"
#include "protocol/edax.cpp"
#include "protocol/cassio.cpp"
#include "protocol/ggs.cpp"
#include "protocol/gtp.cpp"
#include "protocol/nboard.cpp"
#include "protocol/xboard.cpp"

/* library: Edax api (not compiled in the edax program) */
#ifdef LIB_BUILD
	#include "api/libedax.cpp"
#endif

/* what Boost needs without exceptions */
#include "util/boost_support.cpp"

/* main */
#include "app/main.cpp"

