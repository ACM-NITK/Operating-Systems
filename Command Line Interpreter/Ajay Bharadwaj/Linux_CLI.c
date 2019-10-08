#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARG_LEN 512 // Maximum number of characters in each argument
#define MAX_ARGS 128 // Maximum number of arguments per command
#define MAX_COMM 10 // Maximum number of commands in 1 input

// Returns 0 is the string is of length 0 or if it consists of only spaces and 1 otherwise
int notSpaces(char* comm, int length)
{
	for (int a = 0; a < length; a++)
	{
		if (*(comm + a) != ' ')
		{
			return 1;
		}
	}

	return 0;
}

// Parses the input and splits it into different commands and arguments and returns the number of commands
int getCommands(char inp[MAX_ARG_LEN * MAX_ARGS], char* arg_null[MAX_COMM][MAX_ARGS])
{
	inp[strlen(inp)-1] = '\0'; // Replaces the newline character at the end with the null character

	char* commands[MAX_COMM];
	int commCount = -1;
	char* comm = strtok(inp, ";");

	while (comm != NULL && commCount < MAX_COMM)
	{
		if (notSpaces(comm, strlen(comm)))
		{
			commands[++commCount] = comm; // Array of commands whose arguments have yet to be split
		}
		comm = strtok(NULL, ";");
	}
	commCount++;

	char* arg;
	int argCount;
	for (int a = 0; a < commCount; a++)
	{
		argCount = -1;
		arg = strtok(commands[a], " ");

		while (arg && argCount < MAX_ARGS-1)
		{
			if (strlen(arg))
			{
				arg_null[a][++argCount] = arg; // 2D array of commands and their arguments
			}
			arg = strtok(NULL, " ");
		}

		arg_null[a][++argCount] = NULL; // Appends NULL at the end as expected by exec()
	}

	return commCount;
}

int main(int argc, char* argv[])
{
	// If batch mode is required the given file is opened otherwise the input stream is set to stdin
	FILE* fileInp = (argc == 1) ? stdin : fopen(argv[1], "r");
	
	int inpSize = MAX_COMM * MAX_ARGS * MAX_ARG_LEN;
	char inp[inpSize];
	
	char* arg_null[MAX_COMM][MAX_ARGS];
	int numComm;

	int pid;
	
	printf("Shell is starting...\n");
	while (1)
	{
		memset(arg_null, 0, sizeof(arg_null)); // Sets the entire 2D array to the null value

		printf("linux_cli> ");

		if (!fgets(inp, inpSize, fileInp)) // If EOF has been reached, the program should terminate
		{
			exit(0);
		}

		if (argc != 1)
		{
			printf("%s", inp); // If in batch mode, the commands to be executed are shown
		}
		
		numComm = getCommands(inp, arg_null);

		for (int a = 0; a < numComm; a++)
		{
			if (!strcmp(arg_null[a][0], "quit") || !strcmp(arg_null[a][0], "exit"))
			{
				exit(0);
			}
			
			if (!strcmp(arg_null[a][0], "cd"))
			{
				chdir(arg_null[a][1]);
				continue;
			}

			pid = fork();

			if (!pid)
			{
				// Child process
				if (execvp(arg_null[a][0], arg_null[a]) == -1)
				{
					printf("Error: Incorrect Commands and/or Arguments\n");
				}
				exit(0);
			}
			
			// Parent process must wait for a bit before continuing otherwise the output would be jumbled
			for (int b = 0; b < 10000000; b++) {}
		}
	}
}
