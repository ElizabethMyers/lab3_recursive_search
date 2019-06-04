my_file_search: my_file_search.c
        gcc -o file_search_threaded my_file_search.c -pthread -Wall -lm

clean:
        rm file_search_threaded


