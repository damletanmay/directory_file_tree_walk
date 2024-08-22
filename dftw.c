#include <ftw.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

// put executable dftw in the $PATH so that it can be run from anywhere

// constants
#define MAX_ARGS 6
#define MIN_ARGS 3
#define FTW_PHYS 1 // this is already defined in ftw.h but for some reaseon intellisense doesn't detect it.

// operations array index will refer to the operations in code below wherever selected_option is referenced
char *operations[] = {"-nf", "-nd", "-sf", "-mv", "-cpx"};

// extensions array index will refer to the extension which is to be ignore during -cpx wherever ignore_extension is referenced
char *extensions[] = {".c", ".txt", ".pdf"};

int selected_option = -1;
int ignore_extension = -1;

// will hold the source and destination string
char *source, *destination, *source_folder;

// used in mv option
// will hold the directories which were not deleted in the nftw's callback function
// because they weren't empty at the time, deleted during show_output in reverse

char **directories_to_delete;
int cleanup_counter = 0; // will keep track of how many directories needs to be cleaned up

// global counter for number of files and directories
int file_counter = 0;
int dir_counter = 0;
int skipped_counter = 0;

// for some reason FTW struct in ftw.h is not being imported so making this same struct from ftw.h
struct FTW
{
    int base;
    int level;
};

// this function will find selected option and verify that it exists
void find_selected_option(int argc, char *argv[])
{
    // lowercase option part of argument

    for (int i = 0; i < strlen(argv[1]); i++)
    {
        argv[1][i] = tolower(argv[1][i]);
    }

    // find selected option
    for (int i = 0; i < 5; i++)
    {
        if (strcmp(operations[i], argv[1]) == 0)
        {
            selected_option = i;
        }
    }

    if (selected_option == -1)
    {
        printf("No Such Option: %s\n", argv[1]);
        printf("Please select a option from below:\n");

        for (int i = 0; i < 5; i++)
        {
            printf("%s\n", operations[i]);
        }

        exit(-1);
    }
}

//  this function will check the number arguments to see if there are enough or not
void check_number_of_arguments(int argc)
{
    switch (selected_option)
    {
    case 0:
        // for -nf option
        if (argc > 3)
        {
            printf("Option -nf requires 3 arguments\n");
            printf("Example dftw -nf <source_folder_path> \n");
            exit(-1);
        }
        break;
    case 1:
        // for -nd option
        if (argc > 3)
        {
            printf("Option -nd requires 3 arguments\n");
            printf("Example dftw -nd <source_folder_path> \n");
            exit(-1);
        }
        break;
    case 2:
        // for -sf option
        if (argc > 3)
        {
            printf("Option -sf requires 3 arguments\n");
            printf("Example dftw -sf <source_folder_path> \n");
            exit(-1);
        }
        break;
    case 3:
        // for -mv option
        if (argc > 4 || argc == 3)
        {
            printf("Option -mv requires 4 arguments\n");
            printf("Example dftw -mv <source_folder_path> <destination_folder_path> \n");
            exit(-1);
        }
        break;
    case 4:
        // for -cpx option

        if (argc > 5 || argc == 3)
        {
            printf("Option -cpx requires 4 arguments\n");
            printf("Example dftw -cpx <source_folder_path> <destination_folder_path> [extension_to_ignore] \n");
            exit(-1);
        }
        break;
    }
}

// this function will remove consecutive forwardslashes from source and destination while initialization
void remove_consecutive_forwardslashes(char *string)
{
    int length = strlen(string);
    for (int i = length - 1; i > 0; i--)
    {
        if (string[i] == '/')
        {
            // end string
            string[i] = '\0';
        }
        else
        {
            break;
        }
    }
}

// this function will initialize global variables source and destination global variables
// this function will also verify that paths are semantically and syntactically correct
// after this function is called, instead of argv, source and destination will be used
void initialize_source_destination_and_verify(char *argv[])
{
    // remove forwardslashes from source path
    source = argv[2];
    remove_consecutive_forwardslashes(source);

    // if option is > 2
    if (selected_option > 2)
    {
        destination = argv[3];
        remove_consecutive_forwardslashes(destination);

        // destination path shouldn't be same as source path
        if (strcmp(source, destination) == 0)
        {
            printf("Source Can't be same as destination \n");
            exit(-1);
        }

        // destination path shouldn't be inside source path
        // if length of destination path is < source, no issues
        // if same, it'll already be caught in initialize_source_destination_and_verify
        // if more, then it needs to be checked as destination might be inside of source

        // keeping this check so no error occurs,
        // since we removed the //es destination and source won't be of different length and have same path
        // eg: dftw -mv ~/Desktop/folder1/ ~/Desktop/folder1
        if (strlen(destination) >= strlen(source))
        {
            int i;
            // find i where point breaks
            for (i = 0; i < strlen(source); i++)
            {
                if (source[i] != destination[i])
                {
                    break;
                }
            }
            // if loop never breaks that means i is now equal to the length of source
            // destination'i index should be '/' that means it goes deep inside source
            // also this can't be the last character as all //es from end are removed
            if (i == strlen(source) && destination[i] == '/')
            {
                printf("Destination can't be inside Source \n");
                exit(-1);
            }
        }
    }
}

// this function will check if a directory exists
void check_dir_exists(struct stat file_info, const char *string)
{

    stat(string, &file_info);
    if (!S_ISDIR(file_info.st_mode))
    {
        printf("No Such Directory:%s\n", string);
        exit(-1);
    }
}

void check_if_absolute_path(const char *string)
{
    // if first character is not '/' then exit
    if (string[0] != '/')
    {
        printf("%s is not the Absoulte Path\n", string);
        exit(-1);
    }
}

// this function will verify options arguments
void verify_options_arguments(int argc, char *argv[])
{
    // for any option selected, the 3rd argument i.e. source must be a valid path
    struct stat f1, f2;
    check_dir_exists(f1, source);
    check_if_absolute_path(source);

    // for option 3 or 4, the destination path needs to be checked as well
    if (selected_option > 2)
    {
        check_dir_exists(f2, destination);
        check_if_absolute_path(destination);
    }

    if (selected_option == 3)
    {
        // for mv option
        // maximum 1000 files data can be held that means that in any given source folders only 1000 subfolders or sub-subfolders are allowed
        directories_to_delete = malloc(sizeof(char *) * 1000);
    }

    // for cpx option
    if (selected_option == 4)
    {
        // find extension
        // if user provided 5th argument
        if (argc == 5)
        {
            // lowercase extension part of argument

            for (int i = 0; i < strlen(argv[4]); i++)
            {
                argv[4][i] = tolower(argv[4][i]);
            }

            // check what extension user want to ignore
            int i;
            for (i = 0; i < 3; i++)
            {
                if (strcmp(extensions[i], argv[4]) == 0)
                {
                    ignore_extension = i;
                }
            }

            if (ignore_extension == -1)
            {
                printf("%s is not a valid extension and cannot be ignored\n", argv[4]);
                exit(-1);
            }
        }
    }
}

// copy file or folder
void copy_entity(struct FTW *ftw_info, const char *file_path, int flag, const struct stat *file_info, const char *entity_name)
{
    // skip level 0
    if (ftw_info->level != 0)
    {
        // get path after source to append to destination path
        int sub_path_length = strlen(file_path) - strlen(source) + 1;
        char *source_sub_path = malloc((sizeof(char) * sub_path_length) + 1);
        // copy sub_path after source
        int i, j = 0;

        for (i = strlen(source); i < strlen(file_path); i++)
        {
            source_sub_path[j] = file_path[i];
            j++;
        }

        source_sub_path[j] = '\0'; // end string

        // make destination path from destination and source_sub_path
        int destination_path_len = strlen(destination) + strlen(source_folder) + strlen(source_sub_path) + 1;
        char *destination_path = malloc((sizeof(char) * destination_path_len) + 1);

        // append destination path and source sub path
        strcpy(destination_path, destination);
        strcat(destination_path, "/");
        strcat(destination_path, source_folder);
        strcat(destination_path, source_sub_path);

        // if directory, make directory
        if (flag == FTW_D)
        {
            int make = mkdir(destination_path, 0700);
            if (make == -1)
            {
                printf("%s, already exists, skipping.\n", destination_path);
            }
            else
            {
                dir_counter++; // increment directory counter}
            }
        }
        // if file, open file and paste data from source
        else if (flag == FTW_F)
        {
            bool to_copy = true;

            // for cpx option check if fail is to be ignored to copy
            if (ignore_extension != -1)
            {
                // for .c extension
                // extract the last n digits from destination path
                int num_chars_to_copy = strlen(extensions[ignore_extension]);

                char *destination_extension = malloc(sizeof(char) * num_chars_to_copy); // allocate memory

                // copy num_chars_to_copy from destination path
                strncpy(destination_extension, destination_path + strlen(destination_path) - num_chars_to_copy, num_chars_to_copy);

                destination_extension[num_chars_to_copy] = '\0'; // ending string

                // lower case the string
                for (int i = 0; i < strlen(destination_extension); i++)
                {
                    destination_extension[i] = tolower(destination_extension[i]);
                }
                if (strcmp(destination_extension, extensions[ignore_extension]) == 0)
                {
                    to_copy = false;
                    skipped_counter += 1; // increment skipped counter
                }
            }

            if (to_copy)
            {
                // make a buffer
                char *all_bytes = malloc((sizeof(char) * file_info->st_size) + 1);

                // open source file, read all bytes
                int source_fd = open(file_path, O_RDONLY);

                if (source_fd == -1)
                {
                    printf("%s file opening failed, skipping\n", file_path);
                    return;
                }

                read(source_fd, all_bytes, file_info->st_size);
                close(source_fd);

                // open destination file, write all bytes
                int destination_fd = open(destination_path, O_CREAT | O_RDWR, 0700);
                if (destination_fd == -1)
                {
                    printf("%s file opening failed, skipping\n", destination_path);
                    return;
                }

                write(destination_fd, all_bytes, file_info->st_size);
                close(destination_fd);

                free(all_bytes);

                file_counter = file_counter + 1; // increment file counter
            }
        }
        free(source_sub_path);
        free(destination_path);
    }
    else
    {
        // if it's the root directory, make the destination folder first and then extract source path directory name
        char *destination_path = malloc(sizeof(char) * (strlen(destination) + 1 + strlen(entity_name)));
        strcpy(destination_path, destination);
        strcat(destination_path, "/");
        strcat(destination_path, entity_name);

        // make directory
        int make = mkdir(destination_path, 0700);
        if (make == -1)
        {
            printf("%s, already exists, skipping.\n", destination_path);
        }

        // initialize source folder string
        source_folder = malloc(sizeof(char) * (strlen(entity_name) + 2));
        strcpy(source_folder, entity_name);
        free(destination_path);
    }
}

// this function will be recursively called to traverse into files
int callback_function(const char *file_path, const struct stat *file_info, int flag, struct FTW *ftw_info)
{
    // entity can be any thing
    const char *entity_name = file_path + ftw_info->base;

    switch (selected_option)
    {
    case 0:
        // for -nf option
        if (flag == FTW_F)
        {
            file_counter++;
        }
        break;
    case 1:
        // for -nd option
        if (flag == FTW_D && ftw_info->level != 0)
        {
            dir_counter++;
        }
        break;
    case 2:
        // for -sf option
        if (flag == FTW_F)
        {
            printf("File Name: %s, Size in Bytes: %d\n", entity_name, file_info->st_size);
        }
        break;
    case 3:
        // for -mv option

        // step 1 copy
        copy_entity(ftw_info, file_path, flag, file_info, entity_name);

        // step 2 delete files
        int remove_success = remove(file_path);
        if (remove_success == -1)
        {
            // this means that it's a directory and it is not empty and hence can't be removed
            // keep paths of such folders in an array
            // and these folders will be deleted from reverse in show_output function
            directories_to_delete[cleanup_counter] = (char *)malloc(sizeof(char) * strlen(file_path));
            strcpy(directories_to_delete[cleanup_counter], file_path);
            cleanup_counter += 1; // increment cleanup counter
        }
        break;
    case 4:
        // for -cpx option
        copy_entity(ftw_info, file_path, flag, file_info, entity_name);
        break;

    default:
        break;
    }

    return 0;
}

// this function will show output
void show_output()
{
    switch (selected_option)
    {
    case 0:
        // for -nf option
        printf("Total Number of Files in path: %s are %d\n", source, file_counter);
        break;
    case 1:
        // for -nd option
        printf("Total Number of Directories and Sub-Directories in path: %s are %d\n", source, dir_counter);
        break;
    case 2:
        // for -sf option
        // ouput has been taken care of in callback_function
        break;
    case 3:
        // for -mv option
        // once the processing is over, delete files
        for (int i = cleanup_counter - 1; i >= 0; i--)
        {
            int x = remove(directories_to_delete[i]);
            if (x == -1)
            {
                printf("file delete fail\n");
            }
        }

        printf("%d Files and %d Directories moved from: %s to %s\n", file_counter, dir_counter, source, destination);

        break;
    case 4:
        // for -cpx option
        printf("%d Files and %d Directories copied from: %s to %s\n", file_counter, dir_counter, source, destination);
        // if user has chosen to ignore a extension
        if (ignore_extension >= 0)
        {
            printf("%d files ignored having %s extension\n", skipped_counter, extensions[ignore_extension]);
        }

        break;

    default:
        break;
    }
}

// driver function
int main(int argc, char *argv[])
{
    // if only 1 argument i.e. user only entered dftw
    // show documentation
    if (argc == 1)
    {
        char *buffer = malloc(sizeof(char) * 2000);

        int man_fd = open("dftw_man_page.txt", O_RDONLY);

        if (man_fd == -1)
            exit(-1);

        read(man_fd, buffer, 2000);

        printf("%s\n", buffer);

        close(man_fd);
        free(buffer);

        exit(0);
    }

    if (argc < MIN_ARGS)
    {
        printf("You Need to pass More Arguments!\n\n");
        exit(-1);
    }
    else if (argc > MAX_ARGS)
    {
        printf("You Need to pass Less Arguments!\n\n");
        exit(-1);
    }
    else
    {
        find_selected_option(argc, argv);               // find selected option
        check_number_of_arguments(argc);                // check if number of arguments are right according to selected option
        initialize_source_destination_and_verify(argv); // initalize source and destination and verify that they are true
        verify_options_arguments(argc, argv);           // check if source and destination are valid paths and get extension if -cpx option
        // getdtablesize returns maximum number of processes a file can access
        nftw(source, callback_function, getdtablesize(), FTW_PHYS); // recursively call the callback function
        show_output();                                              // show output
    }
}
