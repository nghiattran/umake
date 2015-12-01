#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>

// main

typedef struct commandStruct
{
  char ** commands;
  char * name;
  char ** dependencies;
  int num_dependencies;
  int num_commands;
} commandStruct;

// Given a string and a char, remove all given char from that string
void remove_char(char* base_string, char removeChar);
// Given 2 strings, "abcdef" and "cd", it will return "ab"
char * extract_substring_before_str(char * base_string, char * threshold_string);
// Given 2 strings, "abcdef" and "cd", it will return "ef"
char * extract_substring_after_str(char * base_string, char * threshold_string);
// Set macros for makefile
void set_macros(char* line);
// Count dependancies for an entry in makefile
int count_dependencies(char * line);
// Set dependancies for an entry in makefile
char ** set_dependencies(char * line, int num_dependencies);
// Read info from makefile
struct commandStruct * read_makefile(int * number);
// Given an object name, look for that object in makefile
int search_command_index(struct commandStruct * command_chain, int num_targets, char * object_name);
// Given an object name, execute its commands and its dependencies 
void run_command(struct commandStruct * command_chain, int num_targets, char *object_name);
// Just for testing purposes
void testing(struct commandStruct * command_chain, int num_targets);


int main(int argc, char const *argv[])
{
	struct commandStruct * command_chain;
	int num_targets;
	command_chain = read_makefile(&num_targets);

	if (argc == 1)
	{
		char * command = command_chain[0].name;
		run_command(command_chain, num_targets, command);
		free(command);
	} else if (argc == 2)
	{
		char * command = malloc((strlen(*(argv + 1)) + 1) * sizeof(char));   
		int j;
		for(j = 0; j <= strlen(*(argv + 1)); j++){
	        command[j] = argv[1][j];
	    }

	    if (search_command_index(command_chain, num_targets, command) == -1)
	    {
	    	printf("%s does not exist in makefile.\n", argv[1]);
	    }

	    run_command(command_chain, num_targets, command);
	    free(command);
	} else {
		printf("umake only takes 1 argument %d were given.\n", argc);
	}

	// testing(command_chain, num_targets);

	// deallocate memory
	int i;
	for (i = 0; i < num_targets; ++i)
	{
		free(command_chain[i].commands);
		if (command_chain[i].dependencies != NULL)
		{
			free(command_chain[i].dependencies);
		}
		// printf("here %d\n", i);
		// if (command_chain[i].commands != NULL)
		// {
		// 	free(command_chain[i].commands);
		// }
		// printf("here\n");
		// printf("\n");
	}

	free(command_chain);
	return 0;
}


// Given a string and a char, remove all given char from that string
// Params: base_string and a char to be removed
// Return: None
void remove_char(char* base_string, char removeChar)
{
  char* i = base_string;
  char* j = base_string;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != removeChar)
      i++;
  }
  *i = 0;
}

// Given 2 strings, "abcdef" and "cd", it will return "ab"
// Params: base_string and threshold_string
// Return: string
char * extract_substring_before_str(char * base_string, char * threshold_string)
{
	int num =strstr(base_string,threshold_string) - base_string ;
	char * substr = (char*)malloc(strlen(base_string)+1);
	memcpy(substr, base_string, num +1);
	substr[num] = '\0';
	return substr;
}

// Given 2 strings, "abcdef" and "cd", it will return "ef"
// Params: base_string and threshold_string
// Return: string
char * extract_substring_after_str(char * base_string, char * threshold_string)
{
	int num =strstr(base_string,threshold_string) - base_string ;
	char * substr = (char*)malloc(strlen(base_string)+1);
	sprintf(substr, "%s", base_string + num + 1);
	substr[strlen(base_string) - num - 1] = '\0';
	return substr;
}

// Set macros for makefile
// Params: a string that contains substitution
// Return: None
void set_macros(char* line)
{
	char command[1000];
	remove_char(line, ' ');

	char substr[] = "=";
	int num =strstr(line,"=") - line ;
	char * variableName = extract_substring_before_str(line, substr);
	char * variableValue = extract_substring_after_str(line, substr);
	// printf("%s %s\n", variableName, variableValue);
	setenv(variableName, variableValue, 1) ; 

	free(variableName);
	free(variableValue);
}

// Count dependancies for a entry in makefile
// Params: a string that contains the entry's dependencies
// Return: int, number of dependencies of the entry
int count_dependencies(char * line)
{
	char * dependencies = extract_substring_after_str(line, ":");
	int num_dependencies = 0;
	char * pch = strtok (dependencies," ");
	while (pch != NULL)
	{
		num_dependencies++;
		pch = strtok (NULL, " ");
	}
	free(dependencies);
	// printf("num_dependencies: %d\n", num_dependencies);
	return num_dependencies;
}

// Set dependancies for a entry in makefile
// Params: a string that contains the entry's dependencies and its number of dependencies
// Return: char**, a set of all dependencies
char ** set_dependencies(char * line, int num_dependencies)
{
	// printf("num_dependencies %d\n", num_dependencies);
	if (num_dependencies == 0)
	{
		return NULL;
	}

	char * dependencies_string = extract_substring_after_str(line, ":");
	char ** dependencies = (char**)malloc(num_dependencies* sizeof(char *));
	num_dependencies = 0;

	char * pch = strtok (dependencies_string," ");
	if(pch != NULL)
	{
		dependencies[num_dependencies] = pch;
		// printf("%d %s\n", num_dependencies, dependencies[num_dependencies]);
		num_dependencies++;
	}
	
	while (pch != NULL)
	{
		pch = strtok (NULL, " ");
		if (pch != NULL)
		{
			dependencies[num_dependencies] = pch;
			// printf("%d %s\n", num_dependencies, dependencies[num_dependencies]);
		}
		num_dependencies++;
	}
	// printf("dependencies %s\n", dependencies[0]);
	// free(dependencies_string);
	return dependencies;
}

// Read info from makefile
// Params: uninitialized number, to be set to the number of entries or targets 
// Return: struct commandStruct *, set of all information in makefile
struct commandStruct * read_makefile(int * number)
{
	size_t len = 0;
	ssize_t read;
	char * line;
	FILE * file = fopen("makefile","r");
	int num_targets = 0;
	int num_dependencies[100];
	int num_commands[100];

	while ((read = getline(&line, &len, file)) != -1) {
		size_t ln = strlen(line) - 1;
		if (line[ln] == '\n')
		    line[ln] = '\0';

		if (strstr(line, ":") != NULL)
		{
			// printf("%s\n", line);
			num_commands[num_targets] = 0;
			// printf("num_dependencies: %d\n", count_dependencies(line));
			num_dependencies[num_targets] = count_dependencies(line);
			// printf("num_dependencies: %d\n", num_dependencies[num_targets] );
			num_targets++;
		} else if (line[0] == '	')
		{
			num_commands[num_targets-1]++;
		}
	}

	int i;

	struct commandStruct * command_chain = malloc(sizeof(commandStruct)*num_targets);
	// int i;
	for (i = 0; i < num_targets; i++)
	{
		command_chain[i].commands = (char**)malloc(num_commands[i]* sizeof(char *));
		command_chain[i].dependencies = (char**)malloc(num_dependencies[i]* sizeof(char *));
		command_chain[i].num_dependencies = num_dependencies[i];
		command_chain[i].num_commands = num_commands[i];
		num_dependencies[i] = 0;
		num_commands[i] = 0;
	}
	num_targets = 0;
	fseek ( file , 0 , SEEK_SET );

	while ((read = getline(&line, &len, file)) != -1) {
		size_t ln = strlen(line) - 1;
		if (line[ln] == '\n')
		    line[ln] = '\0';

		// ignore comments
		if (strstr(line, "#") != NULL)
		{
			char * comment = "#";
			line = extract_substring_before_str(line, comment);
		} 
		// set macros
		if (strstr(line, "=") != NULL)
		{
			set_macros(line);
		} 
		// set command_chain
		else if (strstr(line, ":") != NULL)
		{
			command_chain[num_targets].name = extract_substring_before_str(line, ":");
			command_chain[num_targets].dependencies = set_dependencies(line, command_chain[num_targets].num_dependencies);
			if (num_commands[num_targets])
			{
				command_chain[num_targets-1].commands = NULL;
			}
			num_targets++;
		} else if (line[0] == '	')
		{
			remove_char(line, '(');
			remove_char(line, ')');
			// printf("wow %sT\n", line);
			command_chain[num_targets-1].commands[num_commands[num_targets-1]] = (char*)malloc(strlen(line));
			command_chain[num_targets-1].commands[num_commands[num_targets-1]] = line;
			line = NULL;
			num_commands[num_targets-1]++;
		}

		char *tmp = line;
		line = NULL;
		free(tmp);
	}
	
	// testing(command_chain, num_targets);

	fclose(file);

	*number = num_targets;
	return command_chain;
}

// Given an object name, look for that object in makefile
// Params: struct commandStruct * (set of all targets), int (number of targets), char* (a target name)
// Return: int (index of the target)
int search_command_index(struct commandStruct * command_chain, int num_targets, char * object_name)
{
	int i = 0;
	for (; i < num_targets; i++)
	{
		size_t ln = strlen(object_name) - 1;
		if (object_name[ln] == '\n')
		    object_name[ln] = '\0';
		if (strcmp(command_chain[i].name, object_name) == 0)
		{
			return i;
		}
	}
	return -1;
}

// Given an object name, execute its commands and its dependencies 
// Params: struct commandStruct * (set of all targets), int (number of targets), char* (a target name)
// Return: None
void run_command(struct commandStruct * command_chain, int num_targets, char *object_name)
{
	int index = search_command_index(command_chain, num_targets, object_name);

	if (index == -1)
	{
		// printf("Target file does not exist\n");
		// exit(1);
	} else {
		printf("'%s' is running\n", command_chain[index].name);

		if (command_chain[index].num_dependencies > 0)
		{
			int i = 0;
			for (; i < command_chain[index].num_dependencies; i++)
			{
				run_command(command_chain, num_targets, command_chain[index].dependencies[i]);
			}
		}
		int i;
		for (i = 0; i < command_chain[index].num_commands; i++)
		{
			system(command_chain[index].commands[i]);
		}
	}
}

// Just for testing purposes
// Params: struct commandStruct * (set of all targets), int (number of targets), char* (a target name)
// Return: None
void testing(struct commandStruct * command_chain, int num_targets)
{
	int i;
	for (i = 0; i < num_targets; i++)
	{
		printf("target %s ", command_chain[i].name);
		int y;
		for (y = 0; y < command_chain[i].num_dependencies; y++)
		{
			printf(" %s", command_chain[i].dependencies[y]);
		}
		printf("\n");
		for (y = 0; y < command_chain[i].num_commands; y++)
		{
			printf("	%d %d %s\n",y, command_chain[i].num_commands, command_chain[i].commands[y]);
		}
	}
}