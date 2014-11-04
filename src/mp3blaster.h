/* This file is best viewed with tabs being 4 spaces.
 * In this file you can set certain options. 
 */

#ifndef _MP3BLASTER_
#define _MP3BLASTER_

#include <config.h>

/* NCURSES is the location of the ncurses headerfile 
 * I should use config.h that can be generated from autoconf for this
 * hassle, but it's just this ncurses-file that's a known bugger.
 */
#ifdef HAVE_NCURSES_NCURSES_H
#define NCURSES <ncurses/ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#define NCURSES <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#define NCURSES <ncurses.h>
#elif defined(HAVE_CURSES_H)
#define NCURSES <curses.h>
#else
#error "?Ncurses include files not found  error"
#endif 

/* --------------------------------------- */
/* Do not change anything below this line! */
/* --------------------------------------- */

enum playstatus { PS_NORMAL, PS_PLAYING, PS_STOPPED, PS_PAUSED };
enum program_mode { PM_NORMAL, PM_FILESELECTION, PM_HELP, PM_ANY };
enum command_t { 
	CMD_SELECT_FILES, CMD_ADD_GROUP, CMD_LOAD_PLAYLIST, CMD_WRITE_PLAYLIST,
	CMD_SET_GROUP_TITLE, CMD_TOGGLE_REPEAT, CMD_TOGGLE_SHUFFLE, CMD_ENTER,
	CMD_TOGGLE_PLAYMODE, CMD_START_PLAYLIST, CMD_CHANGE_THREAD, CMD_TOGGLE_MIXER,
	CMD_MIXER_VOL_DOWN, CMD_MIXER_VOL_UP, CMD_MOVE_AFTER, CMD_MOVE_BEFORE,
	CMD_QUIT_PROGRAM, CMD_HELP, CMD_DEL, CMD_SELECT, CMD_REFRESH, CMD_PREV_PAGE,
  CMD_NEXT_PAGE, CMD_UP, CMD_DOWN, CMD_FILE_ADD_FILES, CMD_FILE_INV_SELECTION,
	CMD_FILE_RECURSIVE_SELECT, CMD_FILE_SET_PATH, CMD_FILE_DIRS_AS_GROUPS,
	CMD_FILE_MP3_TO_WAV, CMD_FILE_ADD_URL, CMD_FILE_START_SEARCH, CMD_FILE_ENTER,
	CMD_FILE_UP_DIR, CMD_PLAY_PREVIOUS, CMD_PLAY_PLAY, CMD_PLAY_NEXT,
	CMD_PLAY_REWIND, CMD_PLAY_STOP, CMD_PLAY_FORWARD, CMD_NONE,
	CMD_FILE_SELECT, CMD_HELP_PREV, CMD_HELP_NEXT, CMD_FILE_MARK_BAD,
	CMD_CLEAR_PLAYLIST, CMD_DEL_MARK, CMD_FILE_TOGGLE_SORT,
	CMD_FILE_TOGGLE_DISPLAY, CMD_FILE_DELETE, CMD_PLAY_SKIPEND
};

/* how to sort files in dirs ? */
enum sortmodes_t {
	FW_SORT_NONE = 0,
	FW_SORT_ALPHA,
	FW_SORT_ALPHA_CASE,
	FW_SORT_MODIFY_NEW,
	FW_SORT_MODIFY_OLD,
	FW_SORT_SIZE_SMALL,
	FW_SORT_SIZE_BIG
};

enum playmode {
	PLAY_NONE, 	
	PLAY_GROUP,           /* play songs from displayed group in given order */
	PLAY_GROUPS,          /* play all songs from all groups in given order */
	PLAY_GROUPS_RANDOMLY, /* play all songs grouped by groups, with random
	                         group-order */
	PLAY_SONGS };         /* play all songs from all groups in random order */

struct keybind_t {
	int key;
	command_t cmd;
	program_mode pm;
	char desc[20]; //textlength is 19 max, 1 for terminating \0
};

struct keylabel_t
{
	int key;
	char desc[4];
};

/* Structure with global variables used by multiple objects */
struct _globalopts /* global options, exported for other classes */
{
	int no_mixer; /* non-zero if no mixer is wanted */
	int fpl; /* frames to be run in 1 loop: Higher -> less 'hicks' on slow
              * CPU's. Lower -> interface (while playing) reacts faster.
              * 1<=fpl<=10
              */
	int current_group; /* selected group (group_stack[current_group - 1]) */
	short downsample; /* non-zero => downsampling */
	short eightbits;
	char *sound_device;
	sortmodes_t fw_sortingmode;
	short fw_hideothers; /* 0: show all files, 1: hide non-audiofiles */
	short layout; /* different ncurses layouts are possible */
	playmode play_mode;
	unsigned int warndelay;
	unsigned int skipframes;
	short debug;
	short repeat;
	short minimixer; /* 0 for large mixer, 1 for mini mixer */
	struct _colours {
		short
			default_fg, default_bg, popup_fg, popup_bg, popup_input_fg,
			popup_input_bg, error_fg, error_bg, button_fg, button_bg,
			progbar_bg, bartoken_fg, bartoken_bg, shortcut_fg, shortcut_bg,
			label_fg, label_bg, number_fg, number_bg, file_mp3_fg, 
			file_dir_fg, file_lst_fg, file_win_fg;
	} colours;
	short display_mode;
	char **extensions; /* list of regexps that match audiofilenames */
	char **plist_exts; /* list of regexps that list playlistfilenames */
	char *playlist_dir;
#if defined(PTHREADEDMPEG)
	int threads;
#endif
	short want_id3names; /* non-zero if user wants to be able to see id3names */
};

enum keydescs { Main_SelectFiles, Fileman_AddFiles, Playwin_Previous };
//a key is a struct with a keycode(int) and a description-string.
enum input_type { keyboard, plugin };
struct key
{
	int code; /* scancode/value */
	input_type type;
	char plugname[20];
	char desc[4]; //maxlen of desc = 3
};

inline int MIN(int x, int y)
{
	return (x < y ? x : y);
}

inline int MAX(int x, int y)
{
	return (x > y ? x : y);
}

/* interesting prototypes */
#ifdef DEBUG
void debug(const char*,...);
#endif

//void popupWindow(const char*, int, int, int);
void warning(const char*); /* for use with the play-interface */
void Error(const char*); /* for use with the selection-interface */
//void messageBox(const char*);

//colourpair defines used throughout the program
#define CP_DEFAULT 1
#define CP_POPUP   2
#define CP_POPUP_INPUT 3
#define CP_ERROR 4
#define CP_BUTTON 5
#define CP_SHORTCUTS 6
#define CP_LABEL 7
#define CP_NUMBER 8
#define CP_FILE_MP3 9
#define CP_FILE_DIR 10
#define CP_FILE_LST 11
#define CP_FILE_WIN 12

#endif // _MP3BLASTER_
