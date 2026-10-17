/**
 * @file nboard.cpp
 *
 * Chris Welty's NBoard Protocol
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.4
 */

#include "util/options.h"
#include "search/search.h"
#include "util/util.h"
#include "protocol/ui.h"
#include "protocol/play.h"

#include <stdarg.h>

static Log nboard_log[1];

static void nboard_send(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n'); fflush(stdout);

	if (log_is_open(nboard_log)) {
		fprintf(nboard_log->f, "sent> \"");
		va_start(args, format);
		vfprintf(nboard_log->f, format, args);
		va_end(args);
		fprintf(nboard_log->f, "\"\n");
	}
}

static void nboard_fail(const char *format, ...)
{
	va_list args;

	fprintf(stderr, "Error: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	putc('\n', stderr); fflush(stderr);

	if (log_is_open(nboard_log)) {
		fprintf(nboard_log->f, "error> \"");
		va_start(args, format);
		vfprintf(nboard_log->f, format, args);
		va_end(args);
		fprintf(nboard_log->f, "\"\n");
	}
}

static void nboard_send_move(Result *result)
{
	char move[4];

	move_to_string(result->move, WHITE, move);
	nboard_send("=== %s %.2f %.1f", move, 1.0 * result->score, 0.001 * result->time);
}

static void nboard_observer(Result *result)
{
	if (log_is_open(nboard_log)) {
		fprintf(nboard_log->f, "edax> ");
		result_print(result, nboard_log->f);
		putc('\n', nboard_log->f);
	}
	nboard_send("nodestats %llu %.2f", result->n_nodes, 0.001 * result->time);
}

/**
 * @brief initialize edax protocol
 * @param ui user interface
 */
void ui_init_nboard(UI *ui)
{
	Play *play = ui->play;

	// other update...
	play_init(play, &ui->book);
	play->search.options.header = play->search.options.separator = NULL;
	ui->book.search = &play->search;
	book_load_at_startup(&ui->book);
	play->search.id = 1;
	search_set_observer(&play->search, nboard_observer);
	ui->mode = 3;
	play->type = ui->type;

	log_open(nboard_log, options.ui_log_file);
}

/**
 * @brief free resources used by edax protocol
 * @param ui user interface
 */
void ui_free_nboard(UI *ui)
{
	if (ui->book.need_saving) book_save(&ui->book, options.book_file);
	book_free(&ui->book);
	play_free(ui->play);
	log_close(nboard_log);
}

/**
 * @brief Loop event
 * @param ui user interface
 */
void ui_loop_nboard(UI *ui)
{
	char *cmd = NULL, *param = NULL;
	Play *play = ui->play;

	// loop forever
	for (;;) {
		errno = 0;

		if (log_is_open(nboard_log)) {
			play_print(play, nboard_log->f);
		}

		ui_event_wait(ui, &cmd, &param);
		log_print(nboard_log, "received< \"%s %s\"\n", cmd, param);

		if (*cmd == '\0') {

		} else if (string_is(cmd, "nboard")) {
			if (strcmp(param, "1") != 0) nboard_fail("Edax expected \"nboard 1\" protocol");

		} else if (string_is(cmd, "depth")) {
			options.level = string_to_int(param, 21);
			BOUND(options.level, 0, 60, "level");
			options.level_set = true;
			nboard_send("set myname Edax%d", options.level);

		} else if (string_is(cmd, "game")) {
			Game game;
			int lastmove = parse_ggf(&game, param);
			if (lastmove >= 0) {
				game_get_board(&game, 60, &play->initial_board);
				if (lastmove == PASS)	// https://github.com/okuhara/edax-reversi-AVX/issues/1
					board_pass(&play->initial_board);
				play_new(play);
			} else {
				nboard_fail("Cannot parse game \"%s\"", param);
			}

		} else if (string_is(cmd, "move")) {
			if (!play_user_move(play, param)) {
				nboard_fail("Cannot parse move \"%s\"", param);
			}
		} else if (string_is(cmd, "hint")) {

			nboard_send("status Edax is thinking");
			play_hint(play, string_to_int(param, MAX_MOVE));
			nboard_send("status Edax is waiting");

		} else if (string_is(cmd, "go")) {
			nboard_send("status Edax is thinking");
			play_go(play, false);
			nboard_send_move(&play->result);
			nboard_send("status Edax is waiting");

		} else if (string_is(cmd, "quit", "eof")) {
			free(cmd); free(param);			
			return;

		} else if (string_is(cmd, "ping")) {
			nboard_send("pong %s", param);

		} else if (string_is(cmd, "learn")) {
			nboard_send("status Edax is learning");
			play_store(play);
			nboard_send("status Edax is waiting");

		} else if (play_user_move(play, cmd)) {

		// error: unknown message
		} else {
			nboard_fail("unknown command \"%s\" \"%s\"", cmd, param);
		}
	}
}
