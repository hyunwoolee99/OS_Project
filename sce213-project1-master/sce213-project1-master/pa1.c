/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"

static int __process_command(char * command);

/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
LIST_HEAD(history);

struct cmd {
	struct list_head list;
	char* string;
};

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
static int run_command(int nr_tokens, char *tokens[])
{
	if (strcmp(tokens[0], "exit") == 0) return 0; //needs additional code for memory leak issue

	if (strcmp(tokens[0], "cd") == 0)
	{
		if (nr_tokens == 1)
		{
			chdir(getenv("HOME"));
			return 1;
		}
		else if (nr_tokens == 2)
		{
			if (strcmp(tokens[1], "~") == 0) chdir(getenv("HOME"));
			else if (chdir(tokens[1])) fprintf(stderr, "No directory\n");
			return 1;
		}
		return -1;
	}
	if (strcmp(tokens[0], "history") == 0)
	{
		struct cmd *pos;
		int i = 0;
		list_for_each_entry(pos, &history, list)
		{
			fprintf(stderr, "%2d: %s", i, pos->string);
			i++;
		}
		return 1;
	}
	if (strcmp(tokens[0], "!") == 0)
	{
		if (nr_tokens == 2)
		{
			struct cmd *pos;
			int i = 0;
			int num = atoi(tokens[1]);
			char cmd[MAX_COMMAND_LEN] = { "0" };
			pos = list_first_entry(&history, struct cmd, list);
			while (i != num)
			{
				pos = list_next_entry(pos, list);
				i++;
			}
			strcpy(cmd, pos->string);
			__process_command(cmd);
			return 1;
		}
		return -1;
	}
	int i = 0;
	int p_flag = 0;
	int fd[2];
	char *pipe_tokens[MAX_NR_TOKENS] = { NULL };

	while (i < nr_tokens)
	{
		if (strcmp(tokens[i], "|") == 0)
		{
			p_flag = 1;
			i++;
			break;
		}
		i++;
	}

	if (p_flag == 1)
	{
		if (pipe(fd) == -1)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		for (int j = 0; j < i - 1; j++)
		{
			pipe_tokens[j] = (char *)malloc(sizeof(char) * MAX_COMMAND_LEN);
			strcpy(pipe_tokens[j], tokens[j]);
		}
	}

	pid_t pid1 = fork();
	int status1, status2;
	if (pid1 == 0) //child process
	{
		int ret;
		if (p_flag == 1)
		{
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
			ret = execvp(pipe_tokens[0], pipe_tokens);
		}
		else ret = execvp(tokens[0], tokens);
		if (ret < 0)
		{
			//fprintf(stderr, "execution error\n");
			exit(-1);
		}
		else exit(0);
	}
	else if (pid1 > 0) //parent process
	{
		if (p_flag == 1)
		{
			pid_t pid2 = fork();
			if (pid2 == 0)
			{
				int ret;
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);
				close(fd[0]);
				ret = execvp(tokens[i], tokens + i);
				if (ret < 0)
				{
					fprintf(stderr, "execution error\n");
					exit(-1);
				}
				else exit(0);
			}
			close(fd[1]);
			for (int j = 0; j < i; j++)
			{
				free(pipe_tokens[j]);
			}
			wait(&status2);
		}
		wait(&status1);
		if (WEXITSTATUS(status1) == 0)
		{
			if (p_flag == 0) return 1;
			else if (WEXITSTATUS(status2) == 0) return 1;
			return -1;
		}
	}
	fprintf(stderr, "Unable to execute %s\n", tokens[0]);
	return -EINVAL;
}

/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
static void append_history(char * const command)
{
	struct cmd *new_cmd = (struct cmd *)malloc(sizeof(struct cmd));
	INIT_LIST_HEAD(&(new_cmd->list));
	new_cmd->string = (char *)malloc(sizeof(char) * MAX_COMMAND_LEN);
	strcpy(new_cmd->string, command);
	list_add_tail(&(new_cmd->list), &history);
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "^[[0;31;40m";
static const char *__color_end = "^[[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;
	/**
			 * Make stdin unbuffered to prevent ghost (buffered) inputs during
			 * abnormal exit after fork()
			 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		__print_prompt();

		if (!fgets(command, sizeof(command), stdin)) break;

		append_history(command);
		ret = __process_command(command);

		if (!ret) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
