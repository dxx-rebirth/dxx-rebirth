#ifndef _CMD_H_
#define _CMD_H_ 1



/* Maximum length for a single command */
#define CMD_MAX_LENGTH 2048
/* Maximum number of tokens per command */
#define CMD_MAX_TOKENS 64

/* Parse an input string */
void cmd_parse(char *input);

typedef void (*xcommand_t)(void);

/* Warning: these commands are NOT REENTRANT. Do not attempt to thread them! */
void cmd_tokenize(char *string);
int cmd_argc(void);
char *cmd_argv(int w);

#endif /* _CMD_H_ */
