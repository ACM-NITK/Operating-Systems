#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARG_LEN 512
#define MAX_ARGS 128
#define MAX_COMM 10

int notSpaces(char* comm, int length)
{
	if (!length)
	{
		return 0;
	}

	for (int a = 0; a < length; a++)
	{
		if (*(comm + a) != ' ')
		{
			return 1;
		}
	}

	return 0;
}

int getCommands(char inp[MAX_ARG_LEN * MAX_ARGS], char* arg_null[MAX_COMM][MAX_ARGS])
{
	inp[strlen(inp)-1] = '\0';

	char* commands[MAX_COMM];
	int commCount = -1;
	char* comm = strtok(inp, ";");

	while (comm != NULL && commCount < MAX_COMM)
	{
		if (notSpaces(comm, strlen(comm)))
		{
			commands[++commCount] = comm;
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
				arg_null[a][++argCount] = arg;
			}
			arg = strtok(NULL, " ");
		}

		arg_null[a][++argCount] = NULL;
	}

	return commCount;
}

int main(int argc, char* argv[])
{
	FILE* fileInp = (argc == 1) ? stdin : fopen(argv[1], "r");
	
	int inpSize = MAX_COMM * MAX_ARGS * MAX_ARG_LEN;
	char inp[inpSize];
	
	char* arg_null[MAX_COMM][MAX_ARGS];
	int numComm;

	int pid;
	
	printf("Shell is starting...\n");
	while (1)
	{
		memset(arg_null, 0, sizeof(arg_null));

		printf("linux_cli> ");

		if (!fgets(inp, inpSize, fileInp))
		{
			exit(0);
		}

		if (argc != 1)
		{
			printf("%s", inp);
		}
		
		numComm = getCommands(inp, arg_null);

		for (int a = 0; a < numComm; a++)
		{
			if (!strcmp(arg_null[a][0], "quit") || !strcmp(arg_null[a][0], "exit"))
			{
				exit(0);
			}

			pid = fork();

			if (!pid)
			{	
				if (execvp(arg_null[a][0], arg_null[a]) == -1)
				{
					printf("Error: Incorrect Commands and/or Arguments\n");
				}
				exit(0);
			}

			for (int b = 0; b < 10000000; b++) {}
		}
	}
}