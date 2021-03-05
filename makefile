default: compile

compile:
	gcc main.c shell.c jobs.c builtin_commands.c linked_list/linked_list.c -o shell -Wall -pedantic

run:
	./shell
