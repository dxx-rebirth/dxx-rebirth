#include <conf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pstypes.h"
#include "cmd.h"
#include "console.h"
#include "error.h"

/* ======
 * cmd_parse - Parse an input string
 * ======
 */
void cmd_parse(char *input)
{
	char buffer[CMD_MAX_LENGTH];
	char *tokens[CMD_MAX_TOKENS];
	int num_tokens;
	int i, l;

	Assert(input != NULL);
	
	/* Strip leading spaces */
	for (i=0; isspace(input[i]); i++) ;
	strncpy( buffer, &input[i], CMD_MAX_LENGTH );

	printf("lead strip \"%s\"\n",buffer);
	l = strlen(buffer);
	/* If command is empty, give up */
	if (l==0) return;

	/* Strip trailing spaces */
	for (i=l-1; i>0 && isspace(buffer[i]); i--) ;
	buffer[i+1] = 0;
	printf("trail strip \"%s\"\n",buffer);

	/* Split into tokens */
	l = strlen(buffer);
	num_tokens = 1;

	tokens[0] = buffer;
	for (i=1; i<l; i++) {
        	if (isspace(buffer[i])) {
                	buffer[i] = 0;
			while (isspace(buffer[i+1]) && (i+1 < l)) i++;
			tokens[num_tokens++] = &buffer[i+1];
		}
	}

	/* Check for matching commands */

	/* Otherwise */
	printf("n_tokens = %d\n", num_tokens);
	if (num_tokens>1) {
		printf("setting %s %s\n",tokens[0], tokens[1]);
  	  cvar_set(tokens[0], tokens[1]);
	} else {
          con_printf(CON_NORMAL, "%s: %f\n", tokens[0], cvar(tokens[0]));
	}
}
