#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/fs.h>
#include <sys/stat.h>

#define PORT_NUMBER 9000


bool kill_program = false;

void sig_handler(int signum)
{
    if (signum == SIGINT)
    {
        printf("\nCaught signal SIGINT, exiting\n");

        // Syslog the error into the syslog file in /var/log
        syslog(LOG_DEBUG, "Caught signal SIGINT, exiting");

        // Flag to kill the program
        kill_program = true;
    }
    else if (signum == SIGTERM)
    {
        printf("\nCaught signal SIGTERM, exiting\n");

        // Syslog the error into the syslog file in /var/log
        syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting");

        // Flag to kill the program
        kill_program = true;
    }
}

void sig_init()
{
    int status;
    // The sigaction() system call is used to change the action taken by
    // a process on receipt of a specific signal.  (See signal(7) for an
    // overview of signals.)
    // int sigaction(int signum, const struct sigaction *restrict act,
    //               struct sigaction *restrict oldact);
    // struct sigaction 
    // {
    //     void     (*sa_handler)(int);
    //     void     (*sa_sigaction)(int, siginfo_t *, void *);
    //     sigset_t   sa_mask;
    //     int        sa_flags;
    //     void     (*sa_restorer)(void);
    // };
    // manpage: https://man7.org/linux/man-pages/man2/sigaction.2.html
    struct sigaction act;
    memset(&act, 0, sizeof(act));


    // The sigfillset() function shall initialize the signal set pointed
    // to by set, such that all signals defined in this volume of
    // POSIX.1‐2017 are included.
    // int sigfillset(sigset_t *set);
    // manpage: https://man7.org/linux/man-pages/man3/sigfillset.3p.html
    status = sigfillset(&act.sa_mask);
    if (status == -1) // 0 on returned to the child, parent gets the child PID and -1 on error
    {
        printf("\nError: Failed sigfillset(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed sigfillset(). Error code: %d", errno);
        exit (EXIT_FAILURE);
    }


    // Directing the signal actions to sig_handler()
    act.sa_handler = &sig_handler;
    act.sa_flags = 0;

    if(sigaction(SIGINT, &act, NULL) == -1)
	{
        printf("\nError: Failed sigfillset(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed sigfillset(). Error code: %d", errno);
        exit (EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &act, NULL) == -1)
    {
        printf("\nError: Failed sigfillset(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed sigfillset(). Error code: %d", errno);
        exit (EXIT_FAILURE);
    }
}


int main (int argc, char** argv)
{
    printf("AESD Client\n");
    // Appends aesdsocket_client.c to all the logs by default is the program name
    // LOG_PERROR also logs error to stderr
    // Sets the facility to USER
    openlog("aesdsocket_client.c", LOG_PERROR, LOG_USER);

    // Initializing signals and their respective signal handlers
    sig_init();

    int status;

    /*********************************************************************************************************
                                                Daemon Check
    **********************************************************************************************************/
    bool run_as_daemon = false;

    // Checking if the parameters entered are valid
    if(argc == 2)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            run_as_daemon = true;
        }
        else
        {
            printf("\nError: Invalid paramter. Pass '-d' as a command line parameter to create the server as a daemon \
                    process else the server will be created as normal process by default.\n");
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Invalid paramter");
            return -1;
        }
    }

    // @ToDo (Comments these out):
    if (run_as_daemon)
        printf("Daemon: Hello, World from client!\n");
    else
        printf("Normal: Hello, World from client!\n");

    /*********************************************************************************************************
                                                Client Setup
    **********************************************************************************************************/

    // Creating an end point for communication (i.e. socket)
    // int socket(int domain, int type, int protocol);
    // manpage: https://man7.org/linux/man-pages/man2/socket.2.html 
    // Setting SOCK_NONBLOCK enables to use the accept4 or make the socket non block to check for kill_program flag
    int client_socket_fd;
    client_socket_fd = socket(AF_INET, SOCK_STREAM , 0); 
    if (client_socket_fd == -1) // 0 on success and -1 for error
    {
        printf("\nError: Failed socket(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed socket(). Error code: %d", errno);
        return -1;
    }

    // Creating a sockaddr first
    // int getaddrinfo(const char *restrict node,
    //                 const char *restrict service,
    //                 const struct addrinfo *restrict hints,
    //                 struct addrinfo **restrict res);
    // manpage: https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    struct addrinfo hints;
    struct addrinfo *client_info;
    char temp_str[20];

    memset(&hints, 0, sizeof(hints));    // Seting the struct to 0
    hints.ai_family = AF_INET;          // Only IPv4 support
    hints.ai_socktype = SOCK_STREAM;    // TCP stram sockets
    hints.ai_flags = AI_PASSIVE;        // Fill my IP for me

    // if((status = getaddrinfo(NULL, "9000", &hints, &server_info)) != 0)
    sprintf(temp_str, "%d", PORT_NUMBER);         // To convert the PORT_NUMBER into a string
    status = getaddrinfo(NULL, temp_str, &hints, &client_info);
    if(status != 0) // returns 0 if it succeeds or error codes 
    {
        printf("\nError: Failed getaddrinfo(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed getaddrinfo(). Error code: %d", errno);
        return -1;
    }

    // The setsockopt() function shall set the option specified by the
    // option_name argument, at the protocol level specified by the
    // level argument, to the value pointed to by the option_value
    // argument for the socket associated with the file descriptor
    // specified by the socket argument.
    // int setsockopt(int socket, int level, int option_name,
    //                const void *option_value, socklen_t option_len);
    // manpage: https://man7.org/linux/man-pages/man3/setsockopt.3p.html
    int option_value = 1; // Used in setsockopt()
    status = setsockopt(client_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value));
    if(status == -1) // 0 on success and -1 for error
    {
        printf("\nError: Failed setsockopt(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed setsockopt(). Error code: %d", errno);
        freeaddrinfo(client_info);
        return -1;
    }

    // The connect() system call connects the socket referred to by the
    // file descriptor sockfd to the address specified by addr.  The
    // addrlen argument specifies the size of addr.  The format of the
    // address in addr is determined by the address space of the socket
    // sockfd;
    // int connect(int sockfd, const struct sockaddr *addr,
    //             socklen_t addrlen);
    status = connect(client_socket_fd, client_info->ai_addr, client_info->ai_addrlen);
    if(status == -1) // 0 on success and -1 for error
    {
        printf("\nError: Failed connect(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed connect(). Error code: %d", errno);
        freeaddrinfo(client_info);
        return -1;
    }

    // The getsockname() returns the current address to which the socket
    // sockfd is bound, in the buffer pointed to by addr.  The addrlen
    // argument should be initialized to indicate the amount of space
    // (in bytes) pointed to by addr.  On return it contains the actual
    // size of the socket address.
    // int getsockname(int sockfd, struct sockaddr *restrict addr,
    //                 socklen_t *restrict addrlen);
    // manpage: https://man7.org/linux/man-pages/man2/getsockname.2.html
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    char buffer[20];
    const char* ip;
    int p;
    // int server_socket_fd;
    struct sockaddr_in server_sock_addr;
    socklen_t server_sock_len = sizeof(server_sock_addr);
    
    // getpeername() returns the address of the peer connected to the
    // socket sockfd, in the buffer pointed to by addr.  The addrlen
    // argument should be initialized to indicate the amount of space
    // pointed to by addr.  On return it contains the actual size of the
    // name returned (in bytes).  The name is truncated if the buffer
    // provided is too small.
    // int getpeername(int sockfd, struct sockaddr *restrict addr,
    //                 socklen_t *restrict addrlen);
    // manpage: https://man7.org/linux/man-pages/man2/getpeername.2.html
    //Printing the Server IP and Port number
    getpeername(client_socket_fd, (struct sockaddr *)&server_sock_addr, &server_sock_len);
    ip = inet_ntop(AF_INET, &server_sock_addr.sin_addr, buffer, sizeof(buffer));
    // ip = inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)client_info->ai_addr), buffer, sizeof buffer);
    p =  htons(server_sock_addr.sin_port);
    if(ip != NULL) // Converts the IP in to a string to print
    {
        printf("Server IP is : %s :: %d \n" , ip, p);
        // Syslog the info into the syslog file in /var/log
        syslog(LOG_INFO, "Server IP is : %s :: %d \n" , ip, p);
    }


    //Printing the Client IP and Port number
    getsockname(client_socket_fd, (struct sockaddr*) &name, &namelen);
    ip = inet_ntop(AF_INET, &name.sin_addr, buffer, sizeof(buffer));
    p = htons(name.sin_port);
    if(ip != NULL) // Converts the IP in to a string to print
    {
        printf("Client IP is : %s :: %d \n" , ip, p);
        // Syslog the info into the syslog file in /var/log
        syslog(LOG_INFO, "Client IP is : %s :: %d \n" , ip, p);
    }

    

    // The system calls send(), sendto(), and sendmsg() are used to
    // transmit a message to another socket.
    // The send() call may be used only when the socket is in a
    // connected state (so that the intended recipient is known).  The
    // only difference between send() and write(2) is the presence of
    // flags.  With a zero flags argument, send() is equivalent to
    // write(2).
    // ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    // manpage : https://man7.org/linux/man-pages/man2/send.2.html
    
    int file_fd = open("./test.txt", O_RDWR | O_APPEND);

    int file_size = lseek(file_fd, 0, SEEK_END);
    lseek(file_fd, 0, SEEK_SET);
    printf("\nSize of file is: %d bytes\n", file_size);
    char* string = (char*) malloc(file_size);
    int bytes_read = read(file_fd, string, file_size);
    if(bytes_read == -1) // returns -1 on error else number of bytes read
    {
        printf("\nError: Failed read(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed read(). Error code: %d", errno);
        // return -1;
    }
    string[file_size] = 3;

    printf("The string is:\n%s\n", string);

    // char string[] = "Hi my name is nihal\n";
    status = send(client_socket_fd, string, strlen(string), 0);
    if(status == -1) // -1 if error else number of bytes transmitted
    {
         printf("\nError: Failed send(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed send(). Error code: %d", errno);
        freeaddrinfo(client_info);
        return -1;
    }

    printf("\nSent data\n");
    while(!kill_program)
    {

    }
}