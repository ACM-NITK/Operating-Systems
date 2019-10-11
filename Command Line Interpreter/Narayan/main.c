#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// flag=1 when exit statement is encountered
int flag = 0;
//batch_file =1 for batch input
int batch_file = 0;

//turns flag to 1(if there are no arguments for exit), and returns next index to be executed
int exit_program(char **pointer_to_str_arr, int index)
{
	if (pointer_to_str_arr[index + 1] != NULL) //if it has other parameters
	{
		while (pointer_to_str_arr[++index] != NULL)
			;
		printf("\nexit statement doesnt have parameters");
	}
	else
	{
		// exit the program after implementing all other commands
		flag = 1;
		index++;
	}
	return index;
}

// Executes 'cd' command and then returns next index to be checked
int cd(char **pointer_to_str_arr, int i)
{
	// it has to wait for all the child process to finish, as they would have changes the directory structure
	int status = 0, wpid = 0;
	while ((wpid = wait(&status)) > 0)
		;

	if (pointer_to_str_arr[i + 1] == NULL)
	{
		printf("\nToo few arguments for \"cd\"");
	}
	else if (pointer_to_str_arr[i + 2] != NULL)
	{
		printf("\nToo many arguments for \"cd\"");
	}
	else
	{
		if (chdir(pointer_to_str_arr[i + 1]) != 0)
			printf("\nInvalid arguments for \"cd\"");
	}
	while (pointer_to_str_arr[++i] != NULL)
		;
	return i;
}

/*
parses the string into many array of strings seperated by either
   1) NULL pointer to indicate semicolans.
   2) NULL pointer followed by an array with null character as the first element,to indicate end of line
eg:
  cat file ; ls -l
	 will be parsed into [ ['c','a','t'], ['f', 'i', 'l', 'e'], NULL, ['l', 's'], ['-','l'],  NULL,['\0'] ]
*/
char **parse(char *line)
{
	unsigned int BUFFER_SIZE = 10;
	unsigned int arr_size = BUFFER_SIZE, curr_arr_size = 0, next_index_of_line = 0;
	char **rv = (char **)malloc(arr_size * (sizeof(char *)));

	if (!rv)
	{
		printf("\nDynammic allocation failed");
	}
	while (line[next_index_of_line] != 10)
	{
		if (curr_arr_size == (arr_size - 1))
		{
			arr_size += BUFFER_SIZE;
			rv = (char **)realloc(rv, arr_size * (sizeof(char *)));
			if (!rv)
			{
				printf("\nDynammic allocation failed");
			}
		}

		// if it is a semicolon, next pointer is NULL
		if (line[next_index_of_line] == ';')
		{
			rv[curr_arr_size] = NULL;
			next_index_of_line++;
			curr_arr_size++;
			continue;
		}

		//if a word
		int curr_word_index = 0;
		if ((line[next_index_of_line] != ' ') && (line[next_index_of_line] != 10))
		{
			rv[curr_arr_size] = (char *)malloc(20 * sizeof(char)); //assuming each word doesnt exceed 20 chars
			while (line[next_index_of_line] != ';' && line[next_index_of_line] != ' ' && line[next_index_of_line] != 10)
			{
				rv[curr_arr_size][curr_word_index] = line[next_index_of_line];
				curr_word_index++;
				next_index_of_line++;
			}
			rv[curr_arr_size][curr_word_index] = '\0';
			curr_arr_size++;
		}

		//skip all spaces
		if (line[next_index_of_line] == ' ')
			while (line[++next_index_of_line] == ' ')
				;
	}

	rv[curr_arr_size] = NULL;
	curr_arr_size++;
	rv[curr_arr_size] = (char *)malloc(1 * sizeof(char));
	rv[curr_arr_size][0] = '\0';
	return rv;
}

void execute(char **pointer_to_str_arr)
{
	int i = 0;
	pid_t pid = 0;
	int status = 0;
	do
	{
		// each block is of the form [NULLs, a command, NULLs]

		//skip all NULLs
		if (pointer_to_str_arr[i] == NULL)
			while (pointer_to_str_arr[++i] == NULL)
				;

		// for commands
		if (strcmp(pointer_to_str_arr[i], "exit") == 0)
		{
			i = exit_program(pointer_to_str_arr, i);
		}
		else if (strcmp(pointer_to_str_arr[i], "cd") == 0)
		{
			i = cd(pointer_to_str_arr, i);
		}
		else if (pointer_to_str_arr[i][0] != '\0')
		{
			pid = fork();
			if (pid < 0)
			{
				printf("\nError during fork");
				break;
			}
			else if (pid == 0)
			{
				// child process
				printf("\n");
				if (execvp(pointer_to_str_arr[i], &(pointer_to_str_arr[i])) == -1)
					printf("\nError in child process...check your command");
				exit(0);
			}
			else
			{
				// parent process

				while (pointer_to_str_arr[++i] != NULL)
					;
			}
		}

		// skip all NULLs
		if (pointer_to_str_arr[i] == NULL)
			while (pointer_to_str_arr[++i] == NULL)
				;
	} while (pointer_to_str_arr[i][0] != '\0');

	// if parent process, wait for all child processes to complete
	if (pid > 0)
		while ((pid = wait(&status)) > 0)
			;
}

void loop()
{
	char *line;
	char **pointer_to_str_arr;
	int bytes_read;
	while (flag == 0)
	{
		if (!batch_file)
			printf("\n>>");

		long unsigned int LINE_BUFFER_SIZE = 10;
		line = (char *)malloc(LINE_BUFFER_SIZE * sizeof(char));
		bytes_read = getline(&line, &LINE_BUFFER_SIZE, stdin);
		// 'line' will now be pointing to the first character of the string

		// if there was some error in malloc
		if (bytes_read == -1)
		{
			printf("Error");
			continue;
		}
		// if there was nothing in the input
		else if (bytes_read == 1)
		{
			continue;
		}
		int i = 0;

		// pointer_to_str_arr points to an array where each element points to the start of a word
		pointer_to_str_arr = parse(line);
		// execute all the commands in the parsed line
		execute(pointer_to_str_arr);
	}
}

int main(int argc, char **argv)
{
	if (argv[1] != NULL)
	{
		if (argv[2] != NULL)
		{
			printf("Too many arguments");
			exit(0);
		}
		else
		{
			freopen(argv[1], "r", stdin);
			batch_file = 1;
		}
	}
	printf("Entering Shell !!!\n");
	loop();
	printf("\nTerminating...");
	return 0;
}