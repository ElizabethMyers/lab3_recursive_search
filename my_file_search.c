#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

//This program will deal with 4 threads, so declare variable for it here
#define THREAD_NUM 4

//takes a file/dir as argument, recurses, prints name if empty dir or
//not a dir (leaves)
void recur_file_search(char *file);

//share search term globally (rather than passing recursively)
const char *search_term;

int main(int argc, char **argv)
{
   //Usage of this file is command name file_search_threaded,
   //search term <search term>, and starting directory <starting dir>.
   //If there are less or more than these arguments, send message and exit
   if(argc != 3)
   {
	printf("Usage: my_file_search <search_term> <dir>\n");
	printf("Performs recursive file search for files/dirs matching\
		<search_term> starting at <dir>\n");
	exit(1);
   }
   //don't need to bother checking if term/directory are swapped, since we can't
   // know for sure which is which anyway
   search_term = argv[1];

   //open the top-level directory
   DIR *dir = opendir(argv[2]);

   //Declare array of pthreads
   pthread_t thread[THREAD_NUM] = {0};
   //Declare int array to track whether a thread has been initialized
   int threadYes[THREAD_NUM] = {0};

   struct dirent *myFile;


   //make sure top-level dir is openable (i.e., exists and is a directory)
   if(dir == NULL)
   {
   	perror("opendir failed");
	exit(1);
   }

   //start timer for recursive search
   struct timeval start, end;
   gettimeofday(&start, NULL);

   //This will loop through every file or directory in the starting directory
   //until the next item is NULL
   while((myFile = readdir(dir)) != NULL)
   {
      //make sure we don't recurse on . or ..
      if(strcmp(myFile->d_name, "..") != 0 && strcmp(myFile->d_name, ".") != 0)
      {
         //declare a counter to keep track of the current amount of threads
         int threadNum = 0;


         //If threadNum is equal to THREAD_NUM, or 4, wait for these threads to
         //complete before reassigning them to 0
         if(threadNum == THREAD_NUM)
         {
            for(int i = 0; i < THREAD_NUM; i++)
                pthread_join(thread[i], NULL);
             threadNum = 0;
         }


         //The next file is allocated a block of memory in the heap for the
         //length in order to format the file in the directory to be a filepath.
         //The size is the size of myFile and the length of the argument
         //argv[2]+2
         char *nFile = malloc((sizeof(char)*strlen(myFile->d_name)) +
                               strlen(argv[2])+2);
         //Strncpy takes argv[2] with its space given by strlen(argv[2]) which
         //gives the length of the argument and puts it into the previously
         //declared nFile. This print the directory where the search's answers
         //were found
         strncpy(nFile, argv[2], strlen(argv[2]));
         //Strncat appends the characters the length of myFile->d_name and puts
         //them in nFile
	 strncat(nFile, myFile->d_name, strlen(myFile->d_name));
         //This creates new thread in the calling process. &thread[threadNum] is
         //a pointer to a pthread_t structure that pthread_create uses to know
         //which number thread this is in the array. The parameters are set to
         //NULL, and recur_file_search function is called and must return as a
         // void argument. The thread is started with the previously declared
         // nFile which must be a void
	 pthread_create(&thread[threadNum], NULL,(void*)&recur_file_search,
                        (void*)nFile);

         //Increment the thread count by 1 because a new thread has been created
	 threadNum++;
       }
   }

   //Wait for all threads to finish
   for(int i = 0; i < THREAD_NUM; i++)
      pthread_join(thread[i], NULL);

   //Close the directory to dree the DIR pointer and memory for it
   closedir(dir);

   //get the end time and print the time it took to fun the program
   gettimeofday(&end, NULL);
   printf("Time: %ld\n", (end.tv_sec * 1000000 + end.tv_usec)
	- (start.tv_sec * 1000000 + start.tv_usec));

   return 0;
}

//This function takes a path to recurse on, searching for mathes to the
// (global) search_term.  The base case for recursion is when *file is
// not a directory.
//Parameters: the starting path for recursion (char *), which could be a
// directory or a regular file (or something else, but we don't need to
// worry about anything else for this assignment).
//Returns: nothing
//Effects: prints the filename if the base case is reached *and* search_term
// is found in the filename; otherwise, prints the directory name if the
//directory matches search_term.
void recur_file_search(char *file)
{
   //check if directory is actually a file
   DIR *d = opendir(file);
   //NULL means not a directory (or another, unlikely error)
   if(d == NULL)
   {
      //opendir SHOULD error with ENOTDIR, but if it did something else,
      // we have a problem (e.g., forgot to close open files, got
      // EMFILE or ENFILE)
      if(errno != ENOTDIR)
      {
         perror("Something weird happened!");
	 fprintf(stderr, "While looking at: %s\n", file);
	 exit(1);
      }
      //nothing weird happened, check if the file contains the search term
      // and if so print the file to the screen (with full path)
      if(strstr(file, search_term) != NULL)
         printf("%s\n", file);

      //no need to close d (we can't, it is NULL!)
      return;
   }

   //we have a directory, not a file, so check if its name
   // matches the search term
   if(strstr(file, search_term) != NULL)
      printf("%s/\n", file);

   //call recur_file_search for each file in d
   //readdir "discovers" all the files in d, one by one and we
   // recurse on those until we run out (readdir will return NULL)
   struct dirent *cur_file;
   while((cur_file = readdir(d)) != NULL)
   {
      //make sure we don't recurse on . or ..
      if(strcmp(cur_file->d_name, "..") != 0 &&\
 	 strcmp(cur_file->d_name, ".") != 0)
      {
         //we need to pass a full path to the recursive function,
	 // so here we append the discovered filename (cur_file->d_name)
	 // to the current path (file -- we know file is a directory at
	 // this point)
	 char *next_file_str = malloc(sizeof(char) * \
	    strlen(cur_file->d_name) + \
	    strlen(file) + 2);

         strncpy(next_file_str, file, strlen(file));
	 strncpy(next_file_str + strlen(file), \
	    "/", 1);
	 strncpy(next_file_str + strlen(file) + 1, \
	    cur_file->d_name, \
	    strlen(cur_file->d_name) + 1);

         //recurse on the file
	 recur_file_search(next_file_str);

	 //free the dynamically-allocated string
	 free(next_file_str);
      }
   }

   //close the directory, or we will have too many files opened (bad times)
   closedir(d);
}
