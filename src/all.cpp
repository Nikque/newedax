/**
 * @file all.cpp
 *
 * @brief Gather all other files to facilitate compiler inter-procedural optimization.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

/* library (LIB_BUILD): file names in UTF-8 on Windows */
#include "libedax_utf8.h"

/* miscellaneous utilities */
#include "options.cpp"
#include "util.cpp"
#include "stats.cpp"
#include "bit.cpp"

/* move generation */
#include "board.cpp"
#include "move.cpp"

/* eval & search */
#include "eval.cpp"
#include "hash.cpp"
#include "ybwc.cpp"
#include "search.cpp"
#include "endgame.cpp"
#include "midgame.cpp"
#include "root.cpp"

/* miscellaneous tests */
#include "perft.cpp"
#include "obftest.cpp"
#include "histogram.cpp"
#include "bench.cpp"

/* opening book & game database */
#include "book.cpp"
#include "game.cpp"
#include "base.cpp"
#include "opening.cpp"

/* game play with various protocols */
#include "play.cpp"
#include "event.cpp"
#include "ui.cpp"
#include "edax.cpp"
#include "cassio.cpp"
#include "ggs.cpp"
#include "gtp.cpp"
#include "nboard.cpp"
#include "xboard.cpp"

/* library: Edax api (not compiled in the edax program) */
#ifdef LIB_BUILD
	#include "libedax.cpp"
#endif

/* main */
#include "main.cpp"

