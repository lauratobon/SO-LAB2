#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "wish_utils.h"

char error_message2[30] = "An error has occurred\n";

void execute_exit(char *args)
{
    char *path = strtok_r(args, " ", &args);
    if (path != NULL)
    {
        write(STDERR_FILENO, error_message2, strlen(error_message2));
    }
    else
    {
        exit(0);
    }
}

// Implementación del comando cd para cambiar la ruta del directorio actual,sólo debe recibir 1 argumento que será el cambio a la nueva ruta
void execute_cd(char *newpath)
{
    char *path = strtok_r(newpath, " ", &newpath);
    if (path == NULL)
    {
        write(STDERR_FILENO, error_message2, strlen(error_message2));
    }
    else
    {
        if (strtok_r(NULL, " ", &newpath) != NULL)
        {
            write(STDERR_FILENO, error_message2, strlen(error_message2));
        }
        else
        {
            if (access(path, F_OK) == 0) // Si la ruta existe
            {
                chdir(path);
            }
            else
            {
                write(STDERR_FILENO, error_message2, strlen(error_message2));
            }
        }
    }
}

// Implementación del comando path que contiene las rutas donde se buscarán los comandos ejecutables en el programa
// Puede recibir cero(Se conserva el path actual) o más argumentos(se actualiza el path actual con el numero de argumentos que se ingresen)
void execute_path(char *newpath, char ***mypath)
{
    char *copypath = strdup(newpath);
    int path_count = 0;
    int i = 0;
    char *path = strtok_r(copypath, " ", &copypath);

    // count number of paths to be added
    while (path != NULL)
    {
        path_count++;
        path = strtok_r(copypath, " ", &copypath);
    }
    // Path con cero argumentos
    if (path_count == 0)
    {
        i = 0;
        while (strcmp((*mypath)[i], "") != 0)
        {
            (*mypath)[i] = strdup("");
            i++;
        }
    }
    else
    { // Path con n argumentos
        *mypath = realloc(*mypath, (path_count + 1) * sizeof(char *));

        path = strtok_r(newpath, " ", &newpath);
        i = 0;
        while (path != NULL)
        {
            // Verificar si el path es igual a "bin", y si es así, cambiarlo a "/bin/"
            if (strcmp(path, "bin") == 0 || strcmp(path, "/bin") == 0 || strcmp(path, "/bin/") == 0 || strcmp(path, "./bin") == 0 || strcmp(path, "./bin/") == 0)
            {
                (*mypath)[i] = strdup("/bin/");
            }
            else
            {
                if (strstr(path, "./") != path)
                {
                    char *new_path = malloc(strlen(path) + 3); // 2 para "./" y 1 para '\0'
                    strcpy(new_path, "./");
                    strcat(new_path, path);
                    (*mypath)[i] = new_path;
                }
                else
                {
                    (*mypath)[i] = strdup(path);
                }
                // Verificar si el path termina con /
                if (path[strlen(path) - 1] != '/')
                {
                    strcat((*mypath)[i], "/");
                }
            }
            path = strtok_r(newpath, " ", &newpath);
            i++;
        }
        // set last element of mypath to ""
        (*mypath)[i] = "";

        // i = 0;
        // while ((*mypath)[i] != "")
        // {
        // 	printf("NEW paths: %s\n", (*mypath)[i]);
        // 	i++;
        // }
    }
}

int wish_launch_redirect(char **args, char *file)
{
    pid_t pid, wpid;
    int status;
    pid = fork();

    if (pid == 0)
    {
        // Child process
        int fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

        dup2(fd, STDOUT_FILENO); // make stdout go to file
        dup2(fd, STDERR_FILENO); // make stderr go to file - you may choose to not do this

        close(fd);
        execv(args[0], args);
    }
    else if (pid < 0)
    {
        // Error forking
        write(STDERR_FILENO, error_message2, strlen(error_message2));
    }
    else
    {
        // Parent process
        wait(NULL);
    }
    return 1;
}