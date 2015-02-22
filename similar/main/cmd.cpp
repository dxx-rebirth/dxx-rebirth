/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 *
 */
/*
 *
 * Command parsing and processing
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dxxerror.h"
#include "u_mem.h"
#include "strutil.h"
#include "inferno.h"
#include "console.h"
#include "hash.h"


typedef struct cmd_s
{
	const char    *name;
	cmd_handler_t function;
	const char    *help_text;
} cmd_t;

#define CMD_MAX_CMDS 1024

/* The list of cmds */
static hashtable cmd_hash;
static cmd_t *cmd_list[CMD_MAX_CMDS];
static int Num_cmds;


#define ALIAS_NAME_MAX 32
typedef struct cmd_alias_s
{
	char           name[ALIAS_NAME_MAX];
	char           *value;
} cmd_alias_t;

#define CMD_MAX_ALIASES 1024

/* The list of aliases */
static hashtable cmd_alias_hash;
static cmd_alias_t *cmd_alias_list[CMD_MAX_ALIASES];
static int Num_cmd_aliases;


static cmd_t *cmd_findcommand(const char *cmd_name)
{
	int i;

	i = hashtable_search( &cmd_hash, cmd_name );

	if ( i < 0 )
		return NULL;

	return cmd_list[i];
}


static cmd_alias_t *cmd_findalias(const char *alias_name )
{
	int i;

	i = hashtable_search( &cmd_alias_hash, alias_name );

	if ( i < 0 )
		return NULL;

	return cmd_alias_list[i];
}


/* add a new console command */
void cmd_addcommand(const char *cmd_name, cmd_handler_t cmd_func, const char *cmd_help_text)
{
	cmd_t *cmd;
	
	Assert(cmd_name != NULL);

	if (cmd_findcommand(cmd_name))
	{
		Int3();
		con_printf(CON_NORMAL, "command %s already exists, not adding", cmd_name);
		return;
	}

	/* create command, insert to hashtable */
	MALLOC(cmd, cmd_t, 1);
	cmd->name = cmd_name;
	cmd->function = cmd_func;
	cmd->help_text = cmd_help_text;

	hashtable_insert(&cmd_hash, cmd_name, Num_cmds);
	con_printf(CON_DEBUG, "cmd_addcommand: added %s", cmd->name);

	cmd_list[Num_cmds++] = cmd;
}


typedef struct cmd_queue_s
{
	char *command_line;
	struct cmd_queue_s *next;
} cmd_queue_t;

/* The list of commands to be executed */
static cmd_queue_t *cmd_queue_head;
static cmd_queue_t *cmd_queue_tail;


void cvar_cmd_set(int argc, char **argv);


/* execute a parsed command */
void cmd_execute(int argc, char **argv)
{
	cmd_t *cmd;
	cmd_alias_t *alias;

	if ( (cmd = cmd_findcommand(argv[0])) )
	{
		con_printf(CON_DEBUG, "cmd_execute: executing %s", argv[0]);
		cmd->function(argc, argv);
		return;
	}

	if ( (alias = cmd_findalias(argv[0])) && alias->value )
	{
		con_printf(CON_DEBUG, "cmd_execute: pushing alias \"%s\": %s", alias->name, alias->value);
		cmd_insert(alias->value);
		return;
	}
	
	/* Otherwise */
	{  // set value of cvar
		char *new_argv[argc+1];
		int i;
		
		new_argv[0] = (char *)("set");
		for (i = 0; i < argc; i++)
			new_argv[i+1] = argv[i];
		cvar_cmd_set(argc + 1, new_argv);
	}
}


/* Parse an input string */
void cmd_parse(char *input)
{
	char buffer[CMD_MAX_LENGTH];
	char *tokens[CMD_MAX_TOKENS];
	int num_tokens;
	uint_fast32_t i, l;

	Assert(input != NULL);

	/* Strip leading spaces */
	while( isspace(*input) ) { ++input; }
	strncpy( buffer, input, sizeof(buffer) );

	//printf("lead strip \"%s\"\n",buffer);
	l = strlen(buffer);
	/* If command is empty, give up */
	if (l==0) return;
	
	/* Strip trailing spaces */
	for (i=l-1; i>0 && isspace(buffer[i]); i--) ;
	buffer[i+1] = 0;
	//printf("trail strip \"%s\"\n",buffer);
	
	/* Split into tokens */
	l = strlen(buffer);
	num_tokens = 1;
	
	tokens[0] = buffer;
	for (i=1; i<l; i++) {
		if (buffer[i] == '"') {
			tokens[num_tokens - 1] = &buffer[++i];
			while (i < l && buffer[i] != '"')
				i++;
			buffer[i] = 0;
			continue;
		}
		if (isspace(buffer[i]) || buffer[i] == '=') {
			buffer[i] = 0;
			while (isspace(buffer[i+1]) && (i+1 < l)) i++;
			tokens[num_tokens++] = &buffer[i+1];
		}
	}
	
	/* Check for matching commands */
	cmd_execute(num_tokens, tokens);
}


int cmd_queue_wait = 0;

int cmd_queue_process(void)
{
	cmd_queue_t *cmd;
	
	while (!cmd_queue_wait && cmd_queue_head) {
		cmd = cmd_queue_head;
		cmd_queue_head = cmd_queue_head->next;
		if (!cmd_queue_head)
			cmd_queue_tail = NULL;
		
		con_printf(CON_DEBUG, "cmd_queue_process: processing %s", cmd->command_line);
		cmd_parse(cmd->command_line);  // Note, this may change the queue
		
		d_free(cmd->command_line);
		d_free(cmd);
	}
	
	if (cmd_queue_wait > 0) {
		cmd_queue_wait--;
		con_printf(CON_DEBUG, "cmd_queue_process: waiting");
		return 1;
	}
	
	return 0;
}


/* execute until there are no commands left */
void cmd_queue_flush(void)
{
	while (cmd_queue_process()) {
	}
}


/* Add some commands to the queue to be executed */
void cmd_enqueue(int insert, const char *input)
{
	cmd_queue_t *item, *head, *tail;
	char output[CMD_MAX_LENGTH];
	char *optr;
	
	Assert(input != NULL);
	head = tail = NULL;
	
	while (*input) {
		optr = output;
		int quoted = 0;
		
		/* Strip leading spaces */
		while(isspace(*input) || *input == ';')
			input++;
		
		/* If command is empty, give up */
		if (! *input)
			continue;
		
		/* Find the end of this line (\n, ;, or nul) */
		do {
			if (!*input)
				break;
			if (*input == '"') {
				quoted = 1 - quoted;
				continue;
			} else if ( *input == '\n' || (!quoted && *input == ';') ) {
				input++;
				break;
			}
		} while ((*optr++ = *input++));
		*optr = 0;
		
		/* make a new queue item, add it to list */
		MALLOC(item, cmd_queue_t, 1);
		item->command_line = d_strdup(output);
		item->next = NULL;
		
		if (!head)
			head = item;
		if (tail)
			tail->next = item;
		tail = item;
		
		con_printf(CON_DEBUG, "cmd_enqueue: adding %s", output);
	}
	
	if (insert) {
		/* add our list to the head of the main list */
		if (cmd_queue_head)
			tail->next = cmd_queue_head;
		if (!cmd_queue_tail)
			cmd_queue_tail = tail;
		
		cmd_queue_head = head;
		con_printf(CON_DEBUG, "cmd_enqueue: added to front of list");
	} else {
		/* add our list to the tail of the main list */
		if (!cmd_queue_head)
			cmd_queue_head = head;
		if (cmd_queue_tail)
			cmd_queue_tail->next = head;
		
		cmd_queue_tail = tail;
		con_printf(CON_DEBUG, "cmd_enqueue: added to back of list");
	}
}

void cmd_enqueuef(int insert, const char *fmt, ...)
{
	va_list arglist;
	char buf[CMD_MAX_LENGTH];
	
	va_start (arglist, fmt);
	vsnprintf(buf, sizeof(buf), fmt, arglist);
	va_end (arglist);
	
	cmd_enqueue(insert, buf);
}


/* Attempt to autocomplete an input string */
const char *cmd_complete(char *input)
{
	int i;
	uint_fast32_t len = strlen(input);

	if (!len)
		return NULL;

	for (i = 0; i < Num_cmds; i++)
		if (!d_strnicmp(input, cmd_list[i]->name, len))
			return cmd_list[i]->name;

	for (i = 0; i < Num_cmd_aliases; i++)
		if (!d_strnicmp(input, cmd_alias_list[i]->name, len))
			return cmd_alias_list[i]->name;

	return cvar_complete(input);
}


/* alias */
void cmd_alias(int argc, char **argv)
{
	cmd_alias_t *alias;
	char buf[CMD_MAX_LENGTH] = "";
	int i;

	if (argc < 2) {
		con_printf(CON_NORMAL, "aliases:");
		for (i = 0; i < Num_cmd_aliases; i++)
			con_printf(CON_NORMAL, "%s: %s", cmd_alias_list[i]->name, cmd_alias_list[i]->value);
		return;
	}
	
	if (argc == 2) {
		if ( (alias = cmd_findalias(argv[1])) && alias->value )
		{
			con_printf(CON_NORMAL, "%s: %s", alias->name, alias->value);
			return;
		}

		con_printf(CON_NORMAL, "alias: %s not found", argv[1]);
		return;
	}
	
	for (i = 2; i < argc; i++) {
		if (i > 2)
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
		strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 1);
	}

	if ( (alias = cmd_findalias(argv[1])) )
	{
		if ( alias->value )
			d_free(alias->value);
		alias->value = d_strdup(buf);
		return;
	}
	
	MALLOC(alias, cmd_alias_t, 1);
	strncpy(alias->name, argv[1], sizeof(alias->name));
	alias->value = d_strdup(buf);

	hashtable_insert(&cmd_alias_hash, argv[1], Num_cmd_aliases);

	cmd_alias_list[Num_cmd_aliases++] = alias;
}


/* unalias */
void cmd_unalias(int argc, char **argv)
{
	cmd_alias_t *alias;

	if (argc < 2 || argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}

	alias = cmd_findalias(argv[1]);

	if (!alias || !alias->value )
	{
		con_printf(CON_NORMAL, "unalias: %s not found", argv[1]);
		return;
	}

	d_free(alias->value);

	//d_free(alias); // Can't remove from hashtable, so just leave it
}


/* echo to console */
void cmd_echo(int argc, char **argv)
{
	char buf[CMD_MAX_LENGTH] = "";
	int i;

	for (i = 1; i < argc; i++) {
		if (i > 1)
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
		strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 1);
	}
	con_printf(CON_NORMAL, "%s", buf);
}

/* execute script */
void cmd_exec(int argc, char **argv) {
	cmd_queue_t *item, *head, *tail;
	PHYSFSX_gets_line_t<CMD_MAX_LENGTH> line;

	if (argc < 2 || argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}
	
	head = tail = NULL;
	
	auto f = PHYSFSX_openReadBuffered(argv[1]);
	if (!f) {
		con_printf(CON_CRITICAL, "exec: %s not found", argv[1]);
		return;
	}
	while (PHYSFSX_fgets(line, f)) {
		/* make a new queue item, add it to list */
		MALLOC(item, cmd_queue_t, 1);
		item->command_line = d_strdup(line);
		item->next = NULL;
		
		if (!head)
			head = item;
		if (tail)
			tail->next = item;
		tail = item;
		
		con_printf(CON_DEBUG, "cmd_exec: adding %s", static_cast<const char *>(line));
	}
	
	/* add our list to the head of the main list */
	if (cmd_queue_head)
		tail->next = cmd_queue_head;
	if (!cmd_queue_tail)
		cmd_queue_tail = tail;
	
	cmd_queue_head = head;
	con_printf(CON_DEBUG, "cmd_exec: added to front of list");
}


/* get help */
void cmd_help(int argc, char **argv)
{
	cmd_t *cmd;

	if (argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}
	
	if (argc < 2) {
		int i;

		con_printf(CON_NORMAL, "Available commands:");
		for (i = 0; i < Num_cmds; i++)
			con_printf(CON_NORMAL, "    %s", cmd_list[i]->name);

		return;
	}

	cmd = cmd_findcommand(argv[1]);

	if (!cmd) {
		con_printf(CON_URGENT, "Command %s not found", argv[1]);
		return;
	}

	if (!cmd->help_text) {
		con_printf(CON_NORMAL, "%s: no help found", argv[1]);
		return;
	}

	con_printf(CON_NORMAL, "%s", cmd->help_text);
}


/* execute script */
void cmd_wait(int argc, char **argv)
{
	if (argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}
	
	if (argc < 2)
		cmd_queue_wait = 1;
	else
		cmd_queue_wait = atoi(argv[1]);
}


void cmd_free(void)
{
	while (Num_cmds--)
		d_free(cmd_list[Num_cmds]);

	while (Num_cmd_aliases--)
	{
		if (cmd_alias_list[Num_cmd_aliases]->value)
			d_free(cmd_alias_list[Num_cmd_aliases]->value);
		d_free(cmd_alias_list[Num_cmd_aliases]);
	}
}


void cmd_init(void)
{
	cmd_addcommand("alias",     cmd_alias,      "alias <name> <commands>\n" "    define <name> as an alias for <commands>\n"
	                                            "alias <name>\n"            "    show the current definition of <name>\n"
	                                            "alias\n"                   "    show all defined aliases");
	cmd_addcommand("unalias",   cmd_unalias,    "unalias <name>\n"          "    undefine the alias <name>");
	cmd_addcommand("echo",      cmd_echo,       "echo [text]\n"             "    write <text> to the console");
	cmd_addcommand("exec",      cmd_exec,       "exec <file>\n"             "    execute <file>");
	cmd_addcommand("help",      cmd_help,       "help [command]\n"          "    get help for <command>, or list all commands if not specified.");
	cmd_addcommand("wait",      cmd_wait,       "usage: wait [n]\n"         "    stop processing commands, resume in <n> cycles (default 1)");

	atexit(cmd_free);
}
