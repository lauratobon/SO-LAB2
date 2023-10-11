/*
    Ejecucion de multiples subprocesos con batch e interactivo mode
    Para probar, puede usar como ejemplo "ls & pwd & ls -la"

    para compilar: gcc -o wish wish.c wish_utils.c -lreadline
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "wish_utils.h"
#include <libgen.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_COMMANDS 1000
#define MAX_SIZE 500
#define BUFFER_SIZE 1024
#define HISTORY_SIZE 30
// " \t\r\n\a\0"

char e_message[30] = "An error has occurred\n";
char history[HISTORY_SIZE][BUFFER_SIZE];
int reg_history_count = 0; // registro del número de elementos en el historial.

void execute_command(char *command, char ***mypath);

int main(int argc, char *argv[])
{
    // Entrada
    char *input_line;

    // Declaración de mypath
    char **mypath = malloc(2 * sizeof(char *)); // Reserva memoria para dos punteros
    mypath[0] = "/bin/";                        // Asigna la dirección de la cadena "/bin/" al primer elemento del arreglo
    mypath[1] = "";                             // Asigna una cadena vacía al segundo elemento del arreglo

    if (argc == 1) // Ejecución en modo interactivo sin argumentos
    {

        do
        {

            input_line = readline("wish> "); // Leemos una linea de entrada del usuario. Verá wish como indicador del prompt
            if (!input_line)                 // Verificamos que es nulo (EOF o error), en ese caso sale del bucle
            {

                break;
            }
            if (strlen(input_line) > 0) // Verificamos que la linea de entrada no este vacía
            {
                add_history(input_line); // Al no estar vacía se agrega al historial de comandos

                if (reg_history_count < HISTORY_SIZE) // verificamos si el historial aún no está lleno
                {
                    strcpy(history[reg_history_count++], input_line); // Llevamos la entrada a la posición [reg_history_count] del arreglo history
                }                                                     // Incrementamos en 1 reg_history_count
                else
                {
                    for (int i = 0; i < HISTORY_SIZE - 1; i++) // Si esta lleno recorremos history
                    {
                        strcpy(history[i], history[i + 1]); // Y copiamos el penúlyimo en el mas antiguo
                    }
                    strcpy(history[HISTORY_SIZE - 1], input_line); // La nueva entrada queda en la última posoición de history
                }
            }

            input_line[strcspn(input_line, "\n")] = '\0'; // le damos formato a la cadena eliminando caracter de salto de línea (\n)

            execute_command(input_line, &mypath);

        } while (1);
        free(input_line);
    }
    else if (argc == 2) // Ejecución (con argumento) del Modo batch (del archivo bash.txt)
    {
        char commands[MAX_COMMANDS][MAX_SIZE]; // Arreglo para coamandos con cantidad y tamaño delimitados
        int commands_count = 0;                // Contador del numero de comandos leídos

        FILE *fp = fopen(argv[1], "r"); // abrimos el archivo en modo lectura
        if (!fp)                        // Si algo sale mal muestra mensaje de error
        {
            write(STDERR_FILENO, e_message, strlen(e_message));
            return EXIT_FAILURE;
        }

        while (fgets(commands[commands_count], MAX_SIZE, fp)) // Leemos una linea del archivo fp y la guardamos en commands mientras
        {                                                     // fgets retorne un valor distinto de NULL
            commands_count++;                                 // para el while Si retorna NULL, estoindica final del archivo o error de lectura.
        }
        fclose(fp);

        for (int i = 0; i < commands_count; i++) // Iteramos entre los comandos guardados
        {
            input_line = commands[i];
            input_line[strcspn(input_line, "\n")] = '\0'; // le damos formato a la cadena eliminando caracter de salto de línea (\n)
            execute_command(input_line, &mypath);
        }
    }
    else if (argc > 2) // Si hay mas de dos argumentos --mensaje de error
    {
        write(STDERR_FILENO, e_message, strlen(e_message));
        return EXIT_FAILURE;
    }

    return 0; // retorna si el programa terminó sin errores
}

void execute_command(char *command, char ***mypath)
{
    int num_commands = 0;
    int fd;

    // Se verifican si son Built-In, para que no contenga tambien & ya que seria error
    if ((strstr(command, "cd") != 0 || strstr(command, "path") != 0 || strstr(command, "exit") != 0) && strstr(command, "&") == 0)
    {

        // Separamos del comando original el nombre del comando y sus argumentos
        char *n = command;
        char *command_string = strtok_r(n, " ", &n); // dividimos la cadena y separamos pablabras con espacios

        if (strcmp(command_string, "exit") == 0) // verificamos cual de los Built-In se envió como comando y se ejecuta
        {
            execute_exit(n);
        }
        else if (strcmp(command_string, "cd") == 0)
        {
            execute_cd(n);
        }
        else if (strcmp(command_string, "path") == 0)
        {
            execute_path(n, mypath);
        }
    }
    // Comandos restantes que no sean Built-In y son delimitados por "&" para ejecutar en paralelos
    else if (strstr(command, "cd") == NULL && strstr(command, "path") == NULL && strstr(command, "exit") == NULL)
    {

        // Extraccion de todos los posibles comandos
        num_commands = 0;
        char *command_copy = strdup(command);
        char *token = strtok(command_copy, "&");
        while (token != NULL)
        {
            // Verificamos que el comando no esté vacío
            token += strspn(token, " ");
            if (*token != '\0')
            {
                num_commands++;
            }
            token = strtok(NULL, "&");
        }

        char *all_commands[num_commands + 1];
        char *command_copy2 = strdup(command);
        token = strtok(command_copy2, "&");
        int i = 0;
        while (token != NULL)
        {
            /// Verificamos que el comando no esté vacío
            token += strspn(token, " ");
            if (*token != '\0')
            {
                all_commands[i] = token;
                // printf("Commands: %s\n",all_commands[i]);
                i++;
            }
            token = strtok(NULL, "&");
        }
        all_commands[i] = NULL;

        // Inicializamos los pids de los comandos capturados
        pid_t pids[num_commands];

        // Recorrido para todos los comandos que serán lanzados
        for (i = 0; i < num_commands; i++)
        {

            // Separamos del comando original el nombre del comando y sus argumentos
            char *s = all_commands[i];
            char *command_string = strtok_r(s, " ", &s);

            // Iniciamos comprobación del file descriptor basado en buscar las rutas del comando
            fd = -1;
            char ***mp_copy = malloc(2 * sizeof(char **)); // Crea una copia de mypath
            memcpy(mp_copy, mypath, 2 * sizeof(char **));
            char ***mp = mp_copy;
            char specificpath[MAX_SIZE];

            while ((strcmp((*mp)[0], "") != 0) && fd != 0)
            {
                strcpy(specificpath, *(mp[0]++));
                strncat(specificpath, command_string, strlen(command_string));
                fd = access(specificpath, X_OK);
            }
            // Si el file descriptor existe, osea la ruta del programa
            if (fd == 0)
            {
                // Lanzaremos cada comando como un nuevo proceso
                // En caso de error
                if ((pids[i] = fork()) < 0)
                {
                    perror("fork");
                    abort();

                } // Proceso hijo creado
                else if (pids[i] == 0)
                {
                    // Como al hacer fork se hace una copia exacta e independiente de todo el programa padre, puedo manipular variables en el entorno del hijo
                    // Extraigo los argumentos para preparar el execv()

                    // Conteo de los argumentos del comando
                    int num_args = 0;
                    char *s_copy = strdup(s);
                    char *token = strtok(s_copy, " ");
                    while (token != NULL)
                    {
                        num_args++;
                        token = strtok(NULL, " ");
                    }

                    // Guardo los argumentos en el vector myargs
                    i = 1;
                    char *myargs[num_args + 1];
                    myargs[0] = strdup(command_string);
                    char *s_copy2 = strdup(s);
                    token = strtok(s_copy2, " ");
                    while (token != NULL)
                    {
                        myargs[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    myargs[i] = NULL;

                    // FUNCIÓN REDIRECCIÓN
                    int i_found = 0, aux = 0;
                    i = 0;
                    do
                    {

                        if (strcmp(myargs[i], ">") == 0)
                        {
                            aux = aux + 1;
                            i_found = i;
                        }
                        else if (strchr(myargs[i], '>') != NULL)
                        {
                            aux = aux + 2;
                        }

                        i++;
                    } while (myargs[i] != NULL);

                    if (aux == 1)
                    {
                        // encontro 1 >
                        if (myargs[i_found + 1] == NULL)
                        {
                            write(STDERR_FILENO, e_message, strlen(e_message));
                        }
                        else if (myargs[i_found + 2] != NULL)
                        {
                            write(STDERR_FILENO, e_message, strlen(e_message));
                        }
                        else
                        {
                            char *file = malloc(MAX_SIZE * sizeof(char *));
                            strcpy(file, myargs[i_found + 1]);
                            myargs[i_found] = NULL;
                            myargs[i_found + 1] = NULL;
                            if (file != NULL)
                            {

                                myargs[0] = strdup(specificpath);
                                wish_launch_redirect(myargs, file);
                            }
                            else
                            {
                                write(STDERR_FILENO, e_message, strlen(e_message));
                            }
                        }
                    }
                    else if (aux > 1)
                    {
                        write(STDERR_FILENO, e_message, strlen(e_message));
                    }

                    else
                    {

                        // Lanzo el proceso hijo como un nuevo programa
                        // Como me interesa ejecutar el proceso hijo como un nuevo programa, mando los argumentos capturados en el comando sobreescribiendo la imagen del proceso en cuestión
                        execv(specificpath, myargs);
                    }

                    // Si execv() es exitoso, lo siguiente nunca se ejecutará, por precaución para una salida segura
                    exit(EXIT_FAILURE);

                } // Fin del proceso hijo
            }     // Si el file descriptor no existe
            else
            {
                write(STDERR_FILENO, e_message, strlen(e_message));
            }
        } // Fin del for

        // Esperamos que todos los hijos terminen
        int status;
        pid_t pid;
        for (i = 0; i < num_commands; i++)
        {
            pid = wait(&status);
        }
    } // Comando incorrecto
    else
    {
        write(STDERR_FILENO, e_message, strlen(e_message));
    }
}