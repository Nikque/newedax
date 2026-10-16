/**
 * @file options.cpp
 *
 * Options reader.
 *
 * @date 1998 - 2023
 * @author Richard Delorme
 * @version 4.5
 */

#include "util/options.h"
#include "util/stats.h"
#include "util/util.h"
#include "search/search.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

/** global options with default value */
Options options = {
	.hash_table_size = 21, // hash table size (2^21 * 24 * 1.125 = 57MB)
	.inc_sort_depth = {0,-2,-3},
	.n_task = 1, // n_task (will be set to system available cpus at run-time)
	.cpu_affinity = false,
	.verbosity = 1,
	.noise = 0,
	.width = 80,
	.echo = false,
	.info = false,
	.debug_cassio = false,
	.transgress_cassio = true,
	.level = 18,
	.time = TIME_MAX, // infinite time
	.play_type = EDAX_FIXED_LEVEL,
	.can_ponder = true,
	.depth = -1,
	.selectivity = -1,
	.mode = 3,
	.speed = 10000000, // speed (default = 10e6)
	.nps = 0, // nps (default = 0)
	.alpha = SCORE_MIN,
	.beta = SCORE_MAX,
	.all_best = false,
	.eval_file = NULL, // evaluation function's weights file.
	.book_file = NULL,
	.book_allowed = true, // book usage allowed
	.book_randomness = 0,
	.ggs_host = NULL, // ggs host name
	.ggs_login = NULL, // ggs login name
	.ggs_password = NULL,
	.ggs_port = NULL, // port
	.ggs_open = 1, // open
	.probcut_d = 0.25, // probcut depth reduction (/2)
	.pv_debug = false,
	.pv_check = false,
	.pv_guess = false,
	.game_file = NULL, // game file.
	.search_log_file = NULL, // search log file.
	.ui_log_file = NULL, // ui log file.
	.ggs_log_file = NULL, // ggs log file.
	.name = NULL, // edax name
	.auto_start = false,
	.auto_store = false,
	.auto_swap = false,
	.auto_quit = false,
	.repeat = 0,
	.book_save_interval = 60, // minutes between timed book saves
	.book_deviate_save_rounds = 1, // save after every productive deviate round by default
	.book_merge_auto_save = true, // save the merged book after book merge
	.hash_table_auto = false, // hash table size set by hash-table-size (auto: from the thread count and the memory size)
	.book_expand_tasks = 1, // book positions expanded at the same time
	.speed_set = false, // speed given by the user (else measured)
	.level_set = false, // level given by the user (else no level cap with a time control)
	.probcut_model = 0, // probcut model: standard
	.book_depth = 0, // book depth: auto (the depth of the loaded book)
	.book_store_tasks = 0, // games learned at the same time: auto
	.book_store_auto_save = true, // save the book to <book-file>.store after book store
	.book_leaf_recalculate_rounds = 1, // passes of book leaf recalculate
	.book_seed_set = false, // book random numbers from the clock
	.book_seed = 0, // book seed (when set)
	.book_reproducible = false, // book functions as fast as possible (not reproducible)
};

/**
 * @brief Print options usage.
 */
void options_usage(void)
{
	fprintf(stderr, "\nCommon options:\n"
		"  -?|help                       show this message.\n"
		"  -o|option-file                read options from this file.\n"
		"  -v|version                    display the version number.\n"
		"  -name <string>                set Edax name to <string>.\n"
		"  -verbose <n>                  verbosity level.\n"
		"  -q                            silent mode (eq. -verbose 0).\n"
		"  -vv                           very verbose mode (eq. -verbose 2).\n"
		"  -noise <n>                    noise level (print search output from ply <n>).\n"
		"  -width <n>                    line width.\n"
		"  -h|hash-table-size <nbits>    hash table size (2^nbits entries), or auto.\n"
		"  -n|n-tasks <n|auto>           search in parallel using n tasks (auto: the number of logical CPUs).\n"
		"  -cpu                          search using 1 cpu/thread.\n"
#ifdef __APPLE__
		"\nCassio protocol options:\n"
		"  -debug-cassio                 print extra-information in cassio.\n"
		"  -follow-cassio                follow more closely cassio requests.\n"
		"\nOptions unavailable to Cassio protocol\n:"
#endif
		"  -l|level <n>                  search using limited depth.\n"
		"  -t|game-time <n>              search using limited time per game.\n"
		"  -move-time <n>                search using limited time per move.\n"
		"                                (with a time, -l caps the level; without -l, no cap)\n"
		"  -speed <n|auto>               search speed (nodes/s) used to share the time; auto: measured.\n"
		"  -ponder <on/off>              search during opponent time.\n"
		"  -probcut-model <standard|refit> probcut error model (refit: experimental, see README).\n"
		"  -eval-file                    read eval weight from this file.\n"
		"  -book-file                    load opening book from this file.\n"
		"  -book-usage <on/off>          play from the opening book.\n"
		"  -book-depth <n|auto>          depth of the opening book at startup (auto: the depth of the loaded book).\n"
		"  -book-randomness <n>          play various but worse moves from the opening book.\n"
		"  -auto-start <on/off>          automatically restart a new game.\n"
		"  -auto-swap <on/off>           automatically Edax's color between games\n"
		"  -auto-store <on/off>          automatically save played games\n"
		"  -game-file <file>             file to store all played game/s.\n"
		"  -book-save-interval <minutes> minutes between timed book saves (0 disables them).\n"
		"  -book-deviate-save-rounds <n> save deviate progress every n rounds; 0 means completion only.\n"
		"  -book-merge-auto-save <on/off> save the book to <book-file>.mrg after book merge.\n"
		"  -book-expand-tasks <n|auto>   expand n book positions at the same time (n-tasks / n threads each).\n"
		"  -book-store-tasks <n|auto>    learn n games at the same time (n-tasks / n threads each); auto (default): n-tasks; 1: as before.\n"
		"  -book-store-auto-save <on/off> save the book to <book-file>.store after book store and book learn (default on).\n"
		"  -book-leaf-recalculate-rounds <n> passes of book leaf recalculate (2, 3, 4) at most; a pass that changes no leaf is the last (default 1).\n"
		"  -book-seed <n|auto>           seed of the random choices of the book (auto, the default: from the clock).\n"
		"  -book-reproducible <on/off>   book functions give the same book from a run to the next (default off).\n"
		"  -search-log-file <file>       file to store search detailed output/s.\n"
		"  -ui-log-file <file>           file to store input/output to the (U)ser (I)nterface.\n");

	exit(EXIT_SUCCESS);
}

/** true while reading a file of default settings (edax.ini, config.ini) */
static bool reading_defaults = false;

/**
 * @brief Read the integer value of an option.
 *
 * @param option Option name (for the message).
 * @param value Option value: a whole number (spaces after it are accepted).
 * @param current Current value of the option.
 * @return the value, or the current value (with a warning) if the string is not a number.
 */
static int option_int(const char *option, const char *value, const int current)
{
	char *end;
	const long n = strtol(value, &end, 10);

	if (end != value) {
		while (*end == ' ' || *end == '\t') ++end;
		if (*end == '\0') return (int) MAX(INT_MIN, MIN(INT_MAX, n));
	}
	warn("%s: \"%s\" is not a number; ignored\n", option, value);
	return current;
}

/** @brief Same as option_int(), where the word "auto" gives auto_value. */
static int option_int_or_auto(const char *option, const char *value, const int current, const int auto_value)
{
	return strcmp(value, "auto") == 0 ? auto_value : option_int(option, value, current);
}

/**
 * @brief Read book-seed: a whole number from 0 to 2^64 - 1, or auto (the clock).
 *
 * @param option Option name (for the message).
 * @param value Option value.
 */
static void option_seed(const char *option, const char *value)
{
	char *end;
	unsigned long long n;

	if (strcmp(value, "auto") == 0) { options.book_seed_set = false; return; }
	errno = 0;
	n = strtoull(value, &end, 10);
	if (end != value && *value != '-' && errno == 0) {
		while (*end == ' ' || *end == '\t') ++end;
		if (*end == '\0') { options.book_seed = n; options.book_seed_set = true; return; }
	}
	errno = 0;
	warn("%s: \"%s\" is not a number from 0 to %llu, or auto; ignored\n", option, value, ULLONG_MAX);
}

/**
 * @brief Set a string option: the previous string is released (it was lost each time an option
 * was set again, by a second file of settings or by edax_set_option).
 *
 * @param option Option to set.
 * @param value New value (copied).
 */
static void option_string(char **option, const char *value)
{
	char *s = string_duplicate(value);
	free(*option);
	*option = s;
}

/**
 * @brief Read the value of an on/off option (on, off, true, false, yes, no, 1, 0).
 *
 * @param option Option name (for the message).
 * @param value Option value.
 * @param result Option to set (unchanged, with a warning, if the value is not one of these words).
 */
static void option_boolean(const char *option, const char *value, bool *result)
{
	char word[6];
	bool r;

	parse_word(value, word, sizeof word);
	errno = 0;
	r = string_to_boolean(word);
	if (errno == EINVAL) {
		warn("%s: \"%s\" is not on or off; ignored\n", option, value);
		errno = 0;
	} else *result = r;
}

/**
 * @brief Tell if an option has one of some names.
 *
 * @param option Option name.
 * @param names Names.
 * @return true if the option is one of the names.
 */
template <class... Names>
static inline bool option_is(const char *option, const Names... names)
{
	return ((strcmp(option, names) == 0) || ...);
}

/**
 * @brief Read an option.
 *
 * @param option Option name.
 * @param value Option value.
 * @return The number of arguments read (0, 1 or 2).
 */
int options_read(const char *option, const char *value)
{
	int read = 0;

	if (option == NULL) return read;
	while (*option == '-') ++option;
	if (*option == '\0' || *option == '%' || *option == '#') return read;

	read = 1;
	if (option_is(option, "vv")) options.verbosity = 2;
	else if (option_is(option, "q")) options.verbosity = 0;
	else if (option_is(option, "info")) options.info = true;
	else if (option_is(option, "debug-cassio")) options.debug_cassio = true;
	else if (option_is(option, "follow-cassio")) options.transgress_cassio = false;
	else if (option_is(option, "?", "help")) usage();
	else if (option_is(option, "cpu")) options.cpu_affinity = true;
	else {
		read = 0;
		if (value == NULL || *value == '\0') return read;
		read = 2;
		if (option_is(option, "verbose")) options.verbosity = option_int(option, value, options.verbosity);
		else if (option_is(option, "noise")) options.noise = option_int(option, value, options.noise);
		else if (option_is(option, "width")) options.width = option_int(option, value, options.width);

		else if (option_is(option, "h", "hash-table-size")) {
			options.hash_table_auto = (strcmp(value, "auto") == 0);
			if (!options.hash_table_auto) options.hash_table_size = option_int(option, value, options.hash_table_size);
		}
		else if (option_is(option, "n", "n-tasks")) options.n_task = option_int_or_auto(option, value, options.n_task, get_cpu_number());
		else if (option_is(option, "l", "level")) {
			options.level = option_int(option, value, options.level);
			if (!reading_defaults) {	// a level of edax.ini / config.ini is only the initial level
				options.level_set = true;
				options.play_type = EDAX_FIXED_LEVEL;
			}
		} else if (option_is(option, "d", "depth")) {
			options.depth = option_int(option, value, options.depth);
			options.play_type = EDAX_FIXED_LEVEL;
		} else if (option_is(option, "selectivity")) {
			options.selectivity = option_int(option, value, options.selectivity);
			options.play_type = EDAX_FIXED_LEVEL;
		} else if (option_is(option, "t", "game-time")) {
			options.time = string_to_time(value);
			options.play_type = EDAX_TIME_PER_GAME;
		} else if (option_is(option, "move-time")) {
			options.time = string_to_time(value);
			options.play_type = EDAX_TIME_PER_MOVE;
		} else if (option_is(option, "alpha")) options.alpha = option_int(option, value, options.alpha);
		else if (option_is(option, "beta")) options.beta = option_int(option, value, options.beta);
		else if (option_is(option, "all-best")) option_boolean(option, value, &options.all_best);

		else if (option_is(option, "o", "option-file")) options_parse(value);
		else if (option_is(option, "speed")) {
			options.speed_set = (strcmp(value, "auto") != 0);
			if (options.speed_set) options.speed = string_to_real(value, options.speed);
		}
		else if (option_is(option, "nps")) options.nps = 0.001 * string_to_real(value, options.nps);
		else if (option_is(option, "ponder")) option_boolean(option, value, &options.can_ponder);
		else if (option_is(option, "mode")) options.mode = option_int(option, value, options.mode);

		else if (option_is(option, "inc-pvnode-sort-depth")) options.inc_sort_depth[PV_NODE] = option_int(option, value, options.inc_sort_depth[PV_NODE]);
		else if (option_is(option, "inc-cutnode-sort-depth")) options.inc_sort_depth[CUT_NODE] = option_int(option, value, options.inc_sort_depth[CUT_NODE]);
		else if (option_is(option, "inc-allnode-sort-depth")) options.inc_sort_depth[ALL_NODE] = option_int(option, value, options.inc_sort_depth[ALL_NODE]);

		else if (option_is(option, "ggs-host")) option_string(&options.ggs_host, value);
		else if (option_is(option, "ggs-login")) options.ggs_login = string_duplicate(value); // (kept: the GGS client holds this pointer)
		else if (option_is(option, "ggs-password")) option_string(&options.ggs_password, value);
		else if (option_is(option, "ggs-port")) option_string(&options.ggs_port, value);
		else if (option_is(option, "ggs-open")) option_boolean(option, value, &options.ggs_open);

		else if (option_is(option, "probcut-d")) parse_real(value, &options.probcut_d);
		else if (option_is(option, "probcut-model")) {
			if (strcmp(value, "standard") == 0) options.probcut_model = 0;
			else if (strcmp(value, "refit") == 0) options.probcut_model = 1;
			else warn("probcut-model: unknown value \"%s\" (standard or refit)\n", value);
		}

		else if (option_is(option, "pv-debug")) option_boolean(option, value, &options.pv_debug);
		else if (option_is(option, "pv-check")) option_boolean(option, value, &options.pv_check);
		else if (option_is(option, "pv-guess")) option_boolean(option, value, &options.pv_guess);

		else if (option_is(option, "game-file")) option_string(&options.game_file, value);

		else if (option_is(option, "eval-file")) option_string(&options.eval_file, value);	// 11/13/2015

		else if (option_is(option, "book-file")) {
			// the book commands add an extension (".store", ".dev2", ...) to this name, in buffers of FILENAME_MAX characters
			if (strlen(value) > FILENAME_MAX - 8) warn("the name of the book file is too long: ignored\n");
			else option_string(&options.book_file, value);
		}
		else if (option_is(option, "book-usage")) option_boolean(option, value, &options.book_allowed);
		else if (option_is(option, "book-randomness")) options.book_randomness = option_int(option, value, options.book_randomness);

		else if (option_is(option, "search-log-file")) option_string(&options.search_log_file, value);
		else if (option_is(option, "ui-log-file")) option_string(&options.ui_log_file, value);
		else if (option_is(option, "ggs-log-file")) option_string(&options.ggs_log_file, value);

		else if (option_is(option, "name")) option_string(&options.name, value);
		else if (option_is(option, "echo")) option_boolean(option, value, &options.echo);

		else if (option_is(option, "auto-start")) option_boolean(option, value, &options.auto_start);
		else if (option_is(option, "auto-store")) option_boolean(option, value, &options.auto_store);
		else if (option_is(option, "auto-swap")) option_boolean(option, value, &options.auto_swap);
		else if (option_is(option, "auto-quit")) option_boolean(option, value, &options.auto_quit);
		else if (option_is(option, "repeat")) options.repeat = option_int(option, value, options.repeat);
		else if (option_is(option, "book-save-interval")) options.book_save_interval = option_int(option, value, options.book_save_interval);
		else if (option_is(option, "book-deviate-save-rounds")) options.book_deviate_save_rounds = option_int(option, value, options.book_deviate_save_rounds);
		else if (option_is(option, "book-depth")) options.book_depth = option_int_or_auto(option, value, options.book_depth, 0);	// 0 = auto
		else if (option_is(option, "book-expand-tasks")) options.book_expand_tasks = option_int_or_auto(option, value, options.book_expand_tasks, 0);	// 0 = auto
		else if (option_is(option, "book-store-tasks")) options.book_store_tasks = option_int_or_auto(option, value, options.book_store_tasks, 0);	// 0 = auto
		else if (option_is(option, "book-merge-auto-save")) option_boolean(option, value, &options.book_merge_auto_save);
		else if (option_is(option, "book-store-auto-save")) option_boolean(option, value, &options.book_store_auto_save);
		else if (option_is(option, "book-leaf-recalculate-rounds")) options.book_leaf_recalculate_rounds = option_int(option, value, options.book_leaf_recalculate_rounds);
		else if (option_is(option, "book-seed")) option_seed(option, value);
		else if (option_is(option, "book-reproducible")) option_boolean(option, value, &options.book_reproducible);

		else read = 0;
	}

	if (read) {
		info("<set option %s %s>\n", option, value);
	}

	return read;
}


/**
 * @brief Trim the spaces at both ends of a string (in place).
 * @param s String.
 * @return The trimmed string.
 */
static char* settings_trim(char *s)
{
	char *e;
	while (*s == ' ') ++s;
	e = s + strlen(s);
	while (e > s && e[-1] == ' ') --e;
	*e = '\0';
	return s;
}

/**
 * @brief Parse a line of a settings file and apply it.
 *
 * The syntax is tolerant, to make the files easy to edit by hand:
 *  - "name = value", "name=value", "name value" and "set name value" are the same;
 *  - tabs, full-width spaces and a full-width equal sign are accepted, and a UTF-8 BOM is skipped;
 *  - names ignore the case, and spaces, '_' and '-' are the same ("book depth" = "book_depth" = "book-depth");
 *  - the values on/off/true/false/yes/no/auto/standard/refit ignore the case;
 *  - with '=', the value is the rest of the line (a file name may contain spaces);
 *  - '#' starts a comment; an empty line is ignored; an unknown name is reported.
 *
 * @param line Line to parse (modified).
 * @param file File name (for the messages).
 * @param n_line Line number (for the messages).
 */
static void settings_parse_line(char *line, const char *file, int n_line)
{
	static const char *words[] = { "auto", "on", "off", "true", "false", "yes", "no", "standard", "refit", NULL };
	char *s = line, *key, *value, *eq, *p, *q;
	int i, j;

	// normalize the characters: BOM, full-width space and equal sign, tabs, CR
	if ((unsigned char) s[0] == 0xEF && (unsigned char) s[1] == 0xBB && (unsigned char) s[2] == 0xBF) s += 3;
	for (i = j = 0; s[i]; ) {
		const unsigned char c = (unsigned char) s[i];
		if (c == 0xE3 && (unsigned char) s[i + 1] == 0x80 && (unsigned char) s[i + 2] == 0x80) { s[j++] = ' '; i += 3; }
		else if (c == 0xEF && (unsigned char) s[i + 1] == 0xBC && (unsigned char) s[i + 2] == 0x9D) { s[j++] = '='; i += 3; }
		else if (c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f') { s[j++] = ' '; ++i; }
		else s[j++] = s[i++];
	}
	s[j] = '\0';
	if ((p = strchr(s, '#')) != NULL) *p = '\0';	// comment
	s = settings_trim(s);
	if (*s == '\0' || *s == '%') return;

	// split into name and value
	if ((eq = strchr(s, '=')) != NULL) {
		*eq = '\0';
		key = settings_trim(s);
		value = settings_trim(eq + 1);
	} else if ((p = strrchr(s, ' ')) != NULL) {
		*p = '\0';
		key = settings_trim(s);
		value = settings_trim(p + 1);
	} else {
		key = s;
		value = s + strlen(s);
	}

	// name: lower case, spaces and '_' as '-', without a leading "set" or '-'
	for (p = q = key; *p; ++p) {
		char c = (char) tolower((unsigned char) *p);
		if (c == ' ' || c == '_') c = '-';
		if (c == '-' && (q == key || q[-1] == '-')) continue;
		*q++ = c;
	}
	while (q > key && q[-1] == '-') --q;
	*q = '\0';
	if (strncmp(key, "set-", 4) == 0) key += 4;
	if (*key == '\0') return;

	// value: the words of the settings ignore the case
	for (i = 0; words[i]; ++i) {
		for (j = 0; words[i][j] && tolower((unsigned char) value[j]) == words[i][j]; ++j) ;
		if (words[i][j] == '\0' && value[j] == '\0') { strcpy(value, words[i]); break; }
	}

	if (options_read(key, value) == 0) {
		warn("%s:%d: unknown or incomplete setting \"%s\" ignored\n", file, n_line, key);
	}
}

/**
 * @brief parse options from a file
 *
 * @param file Option file name.
 */
void options_parse(const char *file)
{
	// an option file may read another one ("option-file"): a file that reads itself, or a loop of files, is
	// stopped (up to v4.5.5-nikque.15 the stack and the open files were exhausted)
	static int depth = 0;
	char *line;
	FILE *f;

	if (depth >= 16) {
		warn("%s: too many option files read from one another\n", file);
		return;
	}
	f = file_open(file, "r");
	if (f != NULL) {

		int n_line = 0;
		++depth;
		while ((line = string_read_line(f)) != NULL) {
			settings_parse_line(line, file, ++n_line);
			free(line);
		}
		--depth;

		fclose(f);
	}
}

/**
 * @brief parse default settings from a file (edax.ini, config.ini).
 *
 * Same as options_parse(), except that a level only sets the initial level: it does not
 * count as a level given by the user (no level cap with a time control, see play_level()).
 *
 * @param file Option file name.
 */
void options_parse_defaults(const char *file)
{
	reading_defaults = true;
	options_parse(file);
	reading_defaults = false;
}

/**
 * @brief Choose the hash table size from the number of search threads and the memory size.
 *
 * More threads fill the table faster, so it grows with the thread count (one bit when it is
 * multiplied by 4: 21 for 1-3 threads, 22 for 4-15, 23 for 16-63; the gain measured with larger
 * tables was small). The three tables (main + pv + shallow, 27 bytes per main entry) use at most
 * 1/32 of the memory.
 *
 * @param n_task Number of search threads.
 * @return hash table size (in number of bits).
 */
int hash_table_size_auto(const int n_task)
{
	const unsigned long long memory = get_physical_memory();
	int size = 21, n;

	for (n = 4; n <= n_task; n *= 4) ++size;
	if (size > 25) size = 25;
	if (memory) {
		while (size > 21 && (27ULL << size) > memory / 32) --size;
	}
	return size;
}

/**
 * @brief Keep options between realistic values.
 */
void options_bound(void) 
{
	int tmp;
	int max_threads;

	max_threads = MIN(get_cpu_number(), MAX_THREADS);
	BOUND(options.n_task, 1, max_threads, "n-tasks");

	if (options.hash_table_auto) options.hash_table_size = hash_table_size_auto(options.n_task);
	if (sizeof (void*) == 4) {
		BOUND(options.hash_table_size, 10, 25, "hash-table-size");	// 51KB to 1.7GB
	} else {
		BOUND(options.hash_table_size, 10, 30, "hash-table-size");	// 51KB to 53GB
	}

	// 0 = auto. The limit is the largest n-tasks, not its current value: a number given by the user is kept when
	// n-tasks is lowered then raised again (it is capped by n-tasks where it is used).
	if (options.book_expand_tasks != 0) BOUND(options.book_expand_tasks, 1, max_threads, "book-expand-tasks");
	if (options.book_store_tasks != 0) BOUND(options.book_store_tasks, 1, max_threads, "book-store-tasks");
	BOUND(options.book_depth, 0, 60, "book-depth");	// 0 = auto
	BOUND(options.verbosity, 0, 4, "verbosity");
	BOUND(options.mode, 0, 3, "mode"); // (an index of the names of the modes: up to v4.5.5-nikque.15 not bounded)
	BOUND(options.noise, 0, 60, "noise");
	BOUND(options.width, 3, 250, "width");
	BOUND(options.level, 0, 60, "level");
	BOUND(options.time, 1000, TIME_MAX, "time");
	BOUND(options.book_save_interval, 0, 525600, "book-save-interval");
	BOUND(options.book_deviate_save_rounds, 0, 1000000, "book-deviate-save-rounds");
	BOUND(options.book_leaf_recalculate_rounds, 1, 1000000, "book-leaf-recalculate-rounds");

	BOUND(options.alpha, SCORE_MIN, SCORE_MAX, "alpha");
	BOUND(options.beta, SCORE_MIN, SCORE_MAX, "beta");

	BOUND(options.speed, 1e5, 1e12, "speed");

	if (options.alpha > options.beta) {
		fprintf(stderr, "WARNING: alphabeta [%d, %d] will be inverted.\n", options.alpha, options.beta);
		tmp = options.alpha;
		options.alpha = options.beta;
		options.beta = tmp;
	}

	if (options.name == NULL) options.name = string_duplicate(EDAX_NAME);
	if (options.game_file == NULL) options.game_file = string_duplicate("data/game.ggf");
	if (options.eval_file == NULL) options.eval_file = string_duplicate("data/eval.dat");
	if (options.book_file == NULL) options.book_file = string_duplicate("data/book.dat");
}

/**
 * @brief Print all global options.
 * @param f output stream.
 */
void options_dump(FILE *f) 
{
	const char *(play_type[3]) = {"fixed depth", "fixed time per game", "fixed time per move"};
	const char *(boolean_string[2]) = {"false", "true"};
	const char *(mode[4]) = {"human/edax", "edax/human", "edax/edax", "human/human"};	

	fprintf(f, "search display options\n");
	fprintf(f, "\tverbosity: %d\n", options.verbosity);
	fprintf(f, "\tminimal depth (noise): %d\n", options.noise);
	fprintf(f, "\tline width: %d\n", options.width);
	fprintf(f, "\tuser input echo: %s\n", boolean_string[options.echo]);
	fprintf(f, "\t<detailed info>: %s\n\n", boolean_string[options.info]);
	fprintf(f, "Cassio options\n");
	fprintf(f, "\tdisplay debug info in Cassio's 'fenetre de rapport': %s\n", boolean_string[options.debug_cassio]);
	fprintf(f, "\tadapt Cassio requests to search & solve faster: %s\n\n", boolean_string[options.transgress_cassio]);

	fprintf(f, "\tsearch options\n");
	fprintf(f, "\tsize (in number of bits) of the hash table: %d\n", options.hash_table_size);
	fprintf(f, "\tsorting depth increment: pv = %d, all = %d, cut = %d\n",  options.inc_sort_depth[0], options.inc_sort_depth[1], options.inc_sort_depth[2]);
	fprintf(f, "\ttask number for parallel search: %d\n", options.n_task);
	fprintf(f, "\tsearch level: %d\n", options.level);
	fprintf(f, "\tsearch alloted time:"); time_print(options.time, false, stdout); fprintf(f, "\n");
	fprintf(f, "\tsearch with: %s\n", play_type[options.play_type]);
	fprintf(f, "\tsearch pondering: %s\n", boolean_string[options.can_ponder]);
	fprintf(f, "\tsearch depth: %d\n", options.depth);
	fprintf(f, "\tsearch selectivity: %d\n", options.selectivity);
	fprintf(f, "\tsearch speed %.0f N/s\n", options.speed);
	fprintf(f, "\tsearch nps %.0f N/s\n", options.nps);
	fprintf(f, "\tsearch alpha: %d\n", options.alpha);
	fprintf(f, "\tsearch beta: %d\n", options.beta);
	fprintf(f, "\tsearch all best moves: %s\n", boolean_string[options.all_best]);
	fprintf(f, "\teval file: %s\n", options.eval_file);
	fprintf(f, "\tbook file: %s\n", options.book_file);
	fprintf(f, "\tbook allowed: %s\n", boolean_string[options.book_allowed]);
	if (options.book_depth > 0) fprintf(f, "\tbook depth at startup: %d\n", options.book_depth);
	else fprintf(f, "\tbook depth at startup: auto (the depth of the loaded book)\n");
	fprintf(f, "\tbook randomness: %d\n", options.book_randomness);
	fprintf(f, "\tbook timed-save interval: %d minutes\n", options.book_save_interval);
	fprintf(f, "\tbook deviate-save interval: %d productive rounds (0 = completion only)\n", options.book_deviate_save_rounds);
	fprintf(f, "\tbook merge auto-save: %s\n", boolean_string[options.book_merge_auto_save]);
	fprintf(f, "\tbook store auto-save: %s\n", boolean_string[options.book_store_auto_save]);
	fprintf(f, "\tbook leaf recalculate rounds: %d\n", options.book_leaf_recalculate_rounds);
	if (options.book_seed_set) fprintf(f, "\tbook seed: %llu\n", options.book_seed);
	else fprintf(f, "\tbook seed: auto (the clock)\n");
	fprintf(f, "\tbook reproducible: %s\n", boolean_string[options.book_reproducible]);
	if (options.book_store_tasks > 0) fprintf(f, "\tbook store tasks: %d\n\n", options.book_store_tasks);
	else fprintf(f, "\tbook store tasks: auto\n\n");

	fprintf(f, "ggs options\n");
	fprintf(f, "\thost: %s\n", options.ggs_host ? options.ggs_host : "?");
	fprintf(f, "\tport: %s\n", options.ggs_port ? options.ggs_port : "?");
	fprintf(f, "\tlogin: %s\n", options.ggs_login ? options.ggs_login : "?");
	fprintf(f, "\tpassword: %s\n", options.ggs_password ? options.ggs_password : "?");
	fprintf(f, "\topen: %s\n\n", boolean_string[options.ggs_open]);

	fprintf(f, "PV options\n");
	fprintf(f, "\tdebug: %s\n", boolean_string[options.pv_debug]);
	fprintf(f, "\tcheck: %s\n", boolean_string[options.pv_check]);
	fprintf(f, "\tguess: %s\n\n", boolean_string[options.pv_guess]);

	fprintf(f, "game file: %s\n", options.game_file ? options.game_file : "?");

	fprintf(f, "log files\n");
	fprintf(f, "\tsearch: %s\n", options.search_log_file ? options.search_log_file : "?");
	fprintf(f, "\tui: %s\n", options.ui_log_file ? options.ui_log_file : "?");
	fprintf(f, "\tggs: %s\n", options.ggs_log_file ? options.ggs_log_file : "?");

	fprintf(f, "name: %s\n", options.name ? options.name : "?");

	fprintf(f, "Game play\n");
	fprintf(f, "\tmode: %s\n", mode[options.mode]);
	fprintf(f, "\tstart a new game after a game is over: %s\n", boolean_string[options.auto_start]);
	fprintf(f, "\tstore each played game in the opening book: %s\n", boolean_string[options.auto_store]);
	fprintf(f, "\tchange computer's side after each game: %s\n", boolean_string[options.auto_swap]);
	fprintf(f, "\tquit when game is over: %s\n", boolean_string[options.auto_quit]);
	fprintf(f, "\trepeat %d games (before exiting)\n\n\n", options.repeat);
}

/**
 * @brief free allocated resources.
 */
void options_free(void)
{
	free(options.ggs_host);
	free(options.ggs_login);
	free(options.ggs_password);
	free(options.ggs_port);

	free(options.game_file);
	free(options.ui_log_file);
	free(options.search_log_file);
	free(options.ggs_log_file);
	free(options.name);
	free(options.book_file);
	free(options.eval_file);
}

