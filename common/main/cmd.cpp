/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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

#include <cstdarg>
#include <map>
#include <forward_list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dxxerror.h"
#include "u_mem.h"
#include "strutil.h"
#include "inferno.h"
#include "cmd.h"
#include "console.h"
#include "cvar.h"
#include "physfsx.h"

#include "compiler-range_for.h"
#include <memory>

namespace {

struct cmd_t
{
	const char    *name;
	cmd_handler_t function;
	const char    *help_text;
};

/* The list of cmds */
static std::map<const char *, std::unique_ptr<cmd_t>> cmd_list;

#define ALIAS_NAME_MAX 32
struct cmd_alias_t
{
	char           name[ALIAS_NAME_MAX];
	RAIIdmem<char[]> value;
};

}

#define CMD_MAX_ALIASES 1024

/* The list of aliases */
static std::map<std::string, std::unique_ptr<cmd_alias_t>> cmd_alias_list;

static cmd_t *cmd_findcommand(const char *cmd_name)
{
	const auto i = cmd_list.find(cmd_name);
	return i == cmd_list.end() ? nullptr : i->second.get();
}


static cmd_alias_t *cmd_findalias(const char *alias_name )
{
	const auto i = cmd_alias_list.find(alias_name);
	return i == cmd_alias_list.end() ? nullptr : i->second.get();
}


/* add a new console command */
void cmd_addcommand(const char *cmd_name, cmd_handler_t cmd_func, const char *cmd_help_text)
{
	const auto i = cmd_list.insert(std::make_pair(cmd_name, std::unique_ptr<cmd_t>{}));
	if (!i.second)
	{
		Int3();
		con_printf(CON_NORMAL, "command %s already exists, not adding", cmd_name);
		return;
	}
	auto cmd = (i.first->second = std::make_unique<cmd_t>()).get();
	/* create command, insert to hashtable */
	cmd->name = cmd_name;
	cmd->function = cmd_func;
	cmd->help_text = cmd_help_text;

	con_printf(CON_DEBUG, "cmd_addcommand: added %s", cmd->name);
}

namespace {

struct cmd_queue_t
{
	RAIIdmem<char[]> command_line;
	explicit cmd_queue_t(char *p) :
		command_line(p)
	{
	}
};

}

/* The list of commands to be executed */
static std::forward_list<cmd_queue_t> cmd_queue;

/* execute a parsed command */
static void cmd_execute(unsigned long argc, const char *const *const argv)
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
		con_printf(CON_DEBUG, "cmd_execute: pushing alias \"%s\": %s", alias->name, alias->value.get());
		cmd_insert(alias->value.get());
		return;
	}
	
	/* Otherwise */
	if (argc < 31)
	{  // set value of cvar
		const char *new_argv[32];
		int i;
		
		new_argv[0] = "set";
		for (i = 0; i < argc; i++)
			new_argv[i+1] = argv[i];
		cvar_cmd_set(argc + 1, new_argv);
	}
}


/* Parse an input string */
static void cmd_parse(char *input)
{
	char buffer[CMD_MAX_LENGTH];
	char *tokens[CMD_MAX_TOKENS];
	int num_tokens;
	uint_fast32_t i, l;

	Assert(input != NULL);

	/* Strip leading spaces */
	while( isspace(*input) ) { ++input; }
	buffer[sizeof(buffer) - 1] = 0;
	strncpy(buffer, input, sizeof(buffer) - 1);

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


static int cmd_queue_wait;

int cmd_queue_process(void)
{
	for (;;)
	{
		if (cmd_queue_wait)
			break;
		auto cmd = cmd_queue.begin();
		if (cmd == cmd_queue.end())
			break;
		auto command_line = std::move(cmd->command_line);
		cmd_queue.pop_front();
		con_printf(CON_DEBUG, "cmd_queue_process: processing %s", command_line.get());
		cmd_parse(command_line.get());  // Note, this may change the queue
	}
	
	if (cmd_queue_wait > 0) {
		cmd_queue_wait--;
		con_puts(CON_DEBUG, "cmd_queue_process: waiting");
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
	std::forward_list<cmd_queue_t> l;
	char output[CMD_MAX_LENGTH];
	char *optr;
	auto before_end = [](std::forward_list<cmd_queue_t> &f) {
		for (auto i = f.before_begin();;)
		{
			auto j = i;
			if (++ i == f.end())
				return j;
		}
	};
	auto iter = before_end(l);
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
		iter = l.emplace_after(iter, cmd_queue_t{d_strdup(output)});
		con_printf(CON_DEBUG, "cmd_enqueue: adding %s", output);
	}
	auto after = insert
		/* add our list to the head of the main list */
		? (con_puts(CON_DEBUG, "cmd_enqueue: added to front of list"), cmd_queue.before_begin())
		/* add our list to the tail of the main list */
		: (con_puts(CON_DEBUG, "cmd_enqueue: added to back of list"), before_end(cmd_queue))
	;
	cmd_queue.splice_after(after, std::move(l));
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
const char *cmd_complete(const char *input)
{
	uint_fast32_t len = strlen(input);

	if (!len)
		return NULL;

	range_for (const auto &i, cmd_list)
		if (!d_strnicmp(input, i.first, len))
			return i.first;

	range_for (const auto &i, cmd_alias_list)
		if (!d_strnicmp(input, i.first.c_str(), len))
			return i.first.c_str();

	return cvar_complete(input);
}


/* alias */
static void cmd_alias(unsigned long argc, const char *const *const argv)
{
	char buf[CMD_MAX_LENGTH] = "";
	if (argc < 2) {
		con_puts(CON_NORMAL, "aliases:");
		range_for (const auto &i, cmd_alias_list)
			con_printf(CON_NORMAL, "%s: %s", i.first.c_str(), i.second->value.get());
		return;
	}
	
	if (argc == 2) {
		cmd_alias_t *alias;
		if ( (alias = cmd_findalias(argv[1])) && alias->value )
		{
			con_printf(CON_NORMAL, "%s: %s", alias->name, alias->value.get());
			return;
		}

		con_printf(CON_NORMAL, "alias: %s not found", argv[1]);
		return;
	}
	
	for (int i = 2; i < argc; i++) {
		if (i > 2)
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
		strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 1);
	}
	const auto i = cmd_alias_list.insert(std::make_pair(argv[1], std::unique_ptr<cmd_alias_t>{}));
	auto alias = i.first->second.get();
	if (i.second)
	{
		alias = (i.first->second = std::make_unique<cmd_alias_t>()).get();
		alias->name[sizeof(alias->name) - 1] = 0;
		strncpy(alias->name, argv[1], sizeof(alias->name) - 1);
	}
	alias->value.reset(d_strdup(buf));
}


/* unalias */
static void cmd_unalias(unsigned long argc, const char *const *const argv)
{
	if (argc < 2 || argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}

	const char *alias_name = argv[1];
	const auto alias = cmd_alias_list.find(alias_name);
	if (alias == cmd_alias_list.end())
	{
		con_printf(CON_NORMAL, "unalias: %s not found", alias_name);
		return;
	}
	cmd_alias_list.erase(alias);
}

/* echo to console */
static void cmd_echo(unsigned long argc, const char *const *const argv)
{
	char buf[CMD_MAX_LENGTH] = "";
	int i;

	for (i = 1; i < argc; i++) {
		if (i > 1)
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
		strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 1);
	}
	con_puts(CON_NORMAL, buf);
}

/* execute script */
static void cmd_exec(unsigned long argc, const char *const *const argv)
{
	PHYSFSX_gets_line_t<CMD_MAX_LENGTH> line;

	if (argc < 2 || argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}
	auto f = PHYSFSX_openReadBuffered(argv[1]);
	if (!f) {
		con_printf(CON_CRITICAL, "exec: %s not found", argv[1]);
		return;
	}
	std::forward_list<cmd_queue_t> l;
	auto i = l.before_begin();
	while (PHYSFSX_fgets(line, f)) {
		/* make a new queue item, add it to list */
		i = l.emplace_after(i, cmd_queue_t{d_strdup(line)});
		con_printf(CON_DEBUG, "cmd_exec: adding %s", static_cast<const char *>(line));
	}
	
	/* add our list to the head of the main list */
	con_puts(CON_DEBUG, "cmd_exec: added to front of list");
	cmd_queue.splice_after(cmd_queue.before_begin(), std::move(l));
}

/* get help */
static void cmd_help(unsigned long argc, const char *const *const argv)
{
	cmd_t *cmd;

	if (argc > 2) {
		cmd_insertf("help %s", argv[0]);
		return;
	}
	
	if (argc < 2) {
		con_puts(CON_NORMAL, "Available commands:");
		range_for (const auto &i, cmd_list)
			con_printf(CON_NORMAL, "    %s", i.first);

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

	con_puts(CON_NORMAL, cmd->help_text);
}

/* execute script */
static void cmd_wait(unsigned long argc, const char *const *const argv)
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
}
