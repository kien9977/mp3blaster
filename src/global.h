#ifndef _MP3B_GLOBAL_
#define _MP3B_GLOBAL_
#include <stdio.h>

enum cf_error { CF_NONE, TOOMANYVALS, BADVALTYPE, BADKEYWORD, BADVALUE,
				NOSUCHFILE };

char *get_homedir(const char*);
char *expand_path(const char*);
void debug(const char*,...);
char *readline(FILE *);
char *crop_whitespace(const char*);
short is_dir(const char*);
const char *chop_path(const char*);
char *chop_file(const char *a);
int is_mp3(const char*);
int is_wav(const char*);
int is_ogg(const char*);
int is_sid(const char*);
int is_httpstream(const char*);
int is_audiofile(const char *);
int is_playlist(const char *);
char *crunch_string(const char*, unsigned int);
short read_file(const char *filename, char ***lines, int *linecount);
char *id3_filename(const char *filename);

#endif /* _MP3B_GLOBAL_ */
