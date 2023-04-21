/*
This file contains the code for server design.
Reference: https://beej.us/guide/bgnet/html/ 
*/

// ToDo;
// Try to make both buffers local anf free them

#define _GNU_SOURCE

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

#include "../aesd-char-driver/aesd_ioctl.h"

#define DEBUG 1
#define ASCII_NEWLINE 10
#define PORT_NUMBER 9000
#define INIT_BUF_SIZE 1024
#define TIME_PERIOD 10

// A8 build switch
#define USE_AESD_CHAR_DEVICE 1 // Comment for normal AESD socket working

#ifndef USE_AESD_CHAR_DEVICE
char socket_file_path[50] = "/var/tmp/aesdsocketdata";
#else
char socket_file_path[50] = "/dev/aesdchar";
#endif

bool kill_program = false;
bool print_call_program = false;

#ifndef USE_AESD_CHAR_DEVICE

timer_t timer_id;

pthread_mutex_t mutex;

#endif

struct clean_up_data
{
    int server_soc_fd;
    int client_soc_fd;
    char* rx_storage_buff;
};

#ifndef USE_AESD_CHAR_DEVICE
void *print_cal()
{
    struct timespec ts;
    time_t t;
    struct tm* temp;

    while(!kill_program)
    {
        char buffer[INIT_BUF_SIZE] = {0};

        if(0 !=clock_gettime(CLOCK_MONOTONIC, &ts))
        {
            perror("Clock get time failed: ");
            break;
        }
        ts.tv_sec += TIME_PERIOD;

        /* If signal received exit thread */
        if(kill_program)
        {
            pthread_exit(NULL);
        }

        if(0 != clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL))
        {
            perror("clock nanosleep failed: ");
            break;
        }
        /* If signal received exit thread */
        if(kill_program)
        {
            pthread_exit(NULL);
        }
        
        t = time(NULL);
        if (t == ((time_t) -1))
        {
            perror("time since epoch failed: ");
            break;
        }

        temp = localtime(&t);
        if (temp == NULL)
        {
            perror("Local time failed: ");
            break;
        }

        int len = strftime(buffer, INIT_BUF_SIZE, "timestamp: %Y, %b, %d, %H:%M:%S\n", temp);
        if(len == 0)
        {
            syslog(LOG_ERR, "Failed to get time");
        }

        if (0 != pthread_mutex_lock(&mutex)) {
            perror("Mutex lock failed: ");
            break;
        }

        if (-1 == write(socket_file_fd, buffer, len))
        {
            perror("write timestamp failed: ");
        }

        if (pthread_mutex_unlock(&mutex) != 0) {
            perror("Mutex unlock failed: ");
            break;
        }
    }
    return NULL;
}
#endif

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

    // if (sigaction(SIGALRM, &act, NULL) == -1)
    // {
    //     printf("\nError: Failed sigfillset(). Error code: %d\n", errno);
    //     // Syslog the error into the syslog file in /var/log
    //     syslog(LOG_ERR, "Error: Failed sigfillset(). Error code: %d", errno);
    //     exit (EXIT_FAILURE);
    // }
}

void program_kill_clean_up(struct clean_up_data data)
{
    int status;

    // // Closing all the socket file descriptors
    // // sudo lsof -i -P -n | grep LISTEN gives list of ports taht are listening
    // status = close(data.client_soc_fd);
    // if(status != 0) // returns 0 if it succeeds else -1 on error
    // {
    //     printf("\nError: Failed close() the client_socket_fd. Error code: %d\n", errno);
    //     // Syslog the error into the syslog file in /var/log
    //     syslog(LOG_ERR, "Error: Failed close() the client_socket_fd. Error code: %d", errno);
    //     // exit(-1);
    // }

    status = close(data.server_soc_fd);
    if(status != 0) // returns 0 if it succeeds else -1 on error
    {
        printf("\nError: Failed close() the server_socket_fd. Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed close() the server_socket_fd. Error code: %d", errno);
        // exit(-1);
    }

#ifndef USE_AESD_CHAR_DEVICE
    // Unlinking the socket_file_fd to delete it from file system
    // unlink() deletes a name from the filesystem.  If that name was
    // the last link to a file and no processes have the file open, the
    // file is deleted and the space it was using is made available for
    // reuse.
    // int unlink(const char *pathname);
    // manpage: https://man7.org/linux/man-pages/man2/unlink.2.html
    status = unlink(socket_file_path);
    if (status != 0)
    {
        printf("\nError: Failed unlink() the socket_file_fd. Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed unlink() the socket_file_fd. Error code: %d", errno);
        // exit(-1);
    }
#endif

    closelog();

    printf("\n\nSuccessfully Cleaned Up!\nTerminating....\n\nSee you soon and go conquer the world...\n");

    syslog(LOG_INFO, "AESDSOCKET program terminated!!");
    //exit(0);
}

static int daemon_init()
{
    pid_t forked_pid;
    int status;

    /*********************************************************************************************************
                                                Daemon Check
    **********************************************************************************************************/
    // fork() creates a new process by duplicating the calling process.
    // The new process is referred to as the child process.  The calling
    // process is referred to as the parent process.
    // pid_t fork(void);
    // manpage: https://man7.org/linux/man-pages/man2/fork.2.html
    forked_pid = fork();
    if (forked_pid == -1) // 0 on returned to the child, parent gets the child PID and -1 on error
    {
        printf("\nError: Failed fork(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed fork(). Error code: %d", errno);
        return -1;
    }
    // If pork was successful then close parent process
    else if (forked_pid != 0) // parent process executes this
        exit(0);

    // setsid() creates a new session if the calling process is not a
    // process group leader.  The calling process is the leader of the
    // new session (i.e., its session ID is made the same as its process
    // ID).  The calling process also becomes the process group leader
    // of a new process group in the session (i.e., its process group ID
    // is made the same as its process ID).
    // pid_t setsid(void);
    // manpage: https://man7.org/linux/man-pages/man2/setsid.2.html
    status = setsid();
    if (status == -1) // Returns -1 on error and session ID success 
    {
        printf("\nError: Failed setsid(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed setsid(). Error code: %d", errno);
        return -1;
    }

    // chdir() changes the current working directory of the calling
    // process to the directory specified in path.
    // int chdir(const char *path);
    // manpage: https://man7.org/linux/man-pages/man2/chdir.2.html
    status = chdir ("/"); // Changing directory to root
    if (status == -1) // Returns -1 on error and 0 on success 
    {
        printf("\nError: Failed setsid(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed setsid(). Error code: %d", errno);
        return -1;
    }

    // Redirecting the I/O files to NULL file so that the outputs dont come out to the users terminal
    open ("/dev/null", O_RDWR);     // stdin
    dup (0);                        // stdout
    dup (0);                        // stderror

    return 0;
}

typedef struct 
{
    int client_socket_fd;
    bool thread_complete;
    struct sockaddr_in client_sock_addr;
    pthread_t thread_id;
    char* rx_storage_buffer;
    int socket_file_fd;
} thread_data_t;

typedef struct node_s
{
    thread_data_t data;
    struct node_s* next_node;
} node_t;

void *thread_func(void *arg)
{
    thread_data_t* thread_arg = (thread_data_t*)arg;

    int client_socket_fd = thread_arg->client_socket_fd;
    struct sockaddr_in client_sock_addr = thread_arg->client_sock_addr;
    int socket_file_fd_len;
#ifndef USE_AESD_CHAR_DEVICE
    int file_data_size = 0;
#endif
    printf("\n\nNew Connection accepted\n");
    // Syslog the info into the syslog file in /var/log
    syslog(LOG_INFO, "New connection accepted");

    /*********************************************************************************************************
                            Creating or opening the file /var/tmp/aesdsocketdata
    **********************************************************************************************************/
    // The open() system call opens the file specified by pathname.  If
    // the specified file does not exist, it may optionally (if O_CREAT
    // is specified in flags) be created by open().
    // int open(const char *pathname, int flags, mode_t mode);
    // manpage: https://man7.org/linux/man-pages/man2/open.2.html
    
    int socket_file_fd;
    socket_file_fd = open(socket_file_path, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH | S_IWOTH);
    if(socket_file_fd == -1) // returns -1 on error else file descriptor
    {
        printf("\nError: Failed open(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed open(). Error code: %d", errno);
    }
    thread_arg->socket_file_fd = socket_file_fd;

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
    
    //Printing the Server IP and Port number
    getsockname(client_socket_fd, (struct sockaddr*) &name, &namelen);
    ip = inet_ntop(AF_INET, &name.sin_addr, buffer, sizeof(buffer));
    p = htons(name.sin_port);
    if(ip != NULL) // Converts the IP in to a string to print
    {
        printf("Server IP is : %s :: %d \n" , ip, p);
        // Syslog the info into the syslog file in /var/log
        syslog(LOG_INFO, "Server IP is : %s :: %d \n" , ip, p);
    }

    //Printing the Clinet IP and Port number
    ip = inet_ntop(AF_INET, &client_sock_addr.sin_addr, buffer, sizeof(buffer));
    p =  htons(client_sock_addr.sin_port);
    if(ip != NULL) // Converts the IP in to a string to print
    {
        printf("Client IP is : %s :: %d \n" , ip, p);
        // Syslog the info into the syslog file in /var/log
        syslog(LOG_INFO, "Client IP is : %s :: %d \n" , ip, p);
    }
    /*********************************************************************************************************
                                    Receiving Data from client to server
    **********************************************************************************************************/
    // The recv() call is used to receive messages from a socket. They 
    // may be used to receive data on both connectionless and 
    // connection-oriented sockets.
    // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    // manpage: https://man7.org/linux/man-pages/man2/recv.2.html
    char rx_buffer[100];
    char* rx_storage_buffer = NULL;
    int rx_storage_buffer_len = 0;
    int i;
    int rx_data_len;
    bool packet_complete = false;
    
    // This loop will continue to call recv until the packet is complete (\n terminated) 
    // Rationale behind the order of conditions is so that if packet is complete then recv will not even execute
    // changing the order will result in the code to spin and wait for data to be recieved when nothing is available
    while( !packet_complete &&  (rx_data_len = recv(client_socket_fd, rx_buffer, sizeof(rx_buffer), 0))>0)
    {
        for(i=0; i<rx_data_len; ++i)
        {   
            // Looking for new line character
            if(rx_buffer[i] == ASCII_NEWLINE)
            {
                packet_complete = true;
                break;
            }
        }

        if(rx_storage_buffer == NULL)
        {
            rx_storage_buffer = (char*)malloc(i);
            if(rx_storage_buffer == NULL)
            {
                printf("\nError: Failed malloc(). Error code: %d\n", errno);
                // Syslog the error into the syslog file in /var/log
                syslog(LOG_ERR, "Error: Failed malloc(). Error code: %d", errno);
                // return -1;
            }
        }    
        else
        {
            // Will store into temp and then store after sanity check so that the original pointer is not lost
            char* temp = (char*) realloc(rx_storage_buffer, rx_storage_buffer_len+(i));
            if(temp == NULL)
            {
                printf("\nError: Failed realloc(). Error code: %d\n", errno);
                // Syslog the error into the syslog file in /var/log
                syslog(LOG_ERR, "Error: Failed realloc(). Error code: %d", errno);
                free(rx_storage_buffer);
                // return -1;
            }
            else
                rx_storage_buffer = temp;
        }
        memcpy(rx_storage_buffer+(rx_storage_buffer_len), rx_buffer, (i));
        rx_storage_buffer_len += (i);
    }

    // Will store into temp and then store after sanity check so that the original pointer is not lost
    char* temp = (char*) realloc(rx_storage_buffer, rx_storage_buffer_len+1);
    if(temp == NULL)
    {
        printf("\nError: Failed realloc(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed realloc(). Error code: %d", errno);
        free(rx_storage_buffer);
        // return -1;
    }
    else
        rx_storage_buffer = temp;

    *(rx_storage_buffer+(rx_storage_buffer_len)) = '\n';
    rx_storage_buffer_len++;

#if DEBUG
    printf("Data Len Received: %d\n", rx_storage_buffer_len);
    for(i=0;i<rx_storage_buffer_len;i++)
    {
        printf("%c", *(rx_storage_buffer+i));
    }
    printf("\n"); // It seems line the system was buffering the printf so absense of new line was making it buffer and printing only the next time new line was met
    // Refer to https://stackoverflow.com/questions/39180642/why-does-printf-not-produce-any-output
#endif

#ifndef USE_AESD_CHAR_DEVICE
    // The mutex object referenced by mutex shall be locked by a call to
    // pthread_mutex_lock() that returns zero or [EOWNERDEAD].  If the
    // mutex is already locked by another thread, the calling thread
    // shall block until the mutex becomes available.
    // int pthread_mutex_lock(pthread_mutex_t *mutex);
    //  manpage: https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html
    int status = pthread_mutex_lock(&mutex);
    if(status != 0) // returns non zero on error
    {
        printf("\nError: Failed pthread_mutex_lock(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed pthread_mutex_lock(). Error code: %d", errno);
        // return -1;
        goto err_handle;
    }
#endif

    /*********************************************************************************************************
                                    Writing to file /var/tmp/aesdsocketdata
    **********************************************************************************************************/
    const char ioctl_aesdchar_seek_to[] = "AESDCHAR_IOCSEEKTO:";
    int status;
    char *tx_storage_buffer;

    status = strncmp(rx_storage_buffer, ioctl_aesdchar_seek_to, strlen(ioctl_aesdchar_seek_to));
    if(status == 0)
    {
        // When the command is recieved we will call the ioctl

        struct aesd_seekto temp_seekto;

        // Extracting the values of cmd_write and cmd_write_offset from the string
        sscanf(rx_storage_buffer, "AESDCHAR_IOCSEEKTO:%d,%d", &temp_seekto.write_cmd, &temp_seekto.write_cmd_offset);

        status = ioctl(socket_file_fd, AESDCHAR_IOCSEEKTO, &temp_seekto);
        if(status != 0)
        {
            printf("\nError: Failed ioctl(). Error code: %d\n", errno);
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Error: Failed write(). Error code: %d", errno);
            // return -1;
            goto err_handle;
        }

    }
    else
    {
        // No command recieved so we will write to the file
        // write() writes up to count bytes from the buffer starting at buf
        // to the file referred to by the file descriptor fd.
        // ssize_t write(int fd, const void *buf, size_t count);
        // manpage: https://man7.org/linux/man-pages/man2/write.2.html
        int bytes_written = write(socket_file_fd, rx_storage_buffer, rx_storage_buffer_len);
        if(bytes_written == -1) // returns -1 on error else number of bytes written
        {
            printf("\nError: Failed write(). Error code: %d\n", errno);
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Error: Failed write(). Error code: %d", errno);
            // return -1;
            goto err_handle;
        }
    }
    

    


#ifndef USE_AESD_CHAR_DEVICE
    off_t offset;
    // Setting the current file pointer to the start using lseek
    // lseek() repositions the file offset of the open file description
    // associated with the file descriptor fd to the argument offset
    // according to the directive.
    // off_t lseek(int fd, off_t offset, int whence);
    // manpage: https://man7.org/linux/man-pages/man2/lseek.2.html
    offset = lseek(socket_file_fd, 0, SEEK_END); // sets to the begining of the file
    if(offset == -1) // returns -1 on error else number of bytes written
    {
        printf("\nError: Failed lseek(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: lseek lseek(). Error code: %d", errno);
        // return -1;
        goto err_handle;
    }

    /*********************************************************************************************************
                                Preparing to read from file /var/tmp/aesdsocketdata
    **********************************************************************************************************/
    // Setting the current file pointer to the start using lseek
    // lseek() repositions the file offset of the open file description
    // associated with the file descriptor fd to the argument offset
    // according to the directive.
    // off_t lseek(int fd, off_t offset, int whence);
    // manpage: https://man7.org/linux/man-pages/man2/lseek.2.html
    file_data_size = lseek(socket_file_fd, 0, SEEK_SET); // sets to the begining of the file
    if(file_data_size == -1) // returns -1 on error else number of bytes written
    {
        printf("\nError: Failed lseek(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: lseek write(). Error code: %d", errno);
        // return -1;
        goto err_handle;
    }
    socket_file_fd_len = offset;
    // mallocing a buffer big enough to accomodate the entire file's data
    tx_storage_buffer = (char*)malloc(socket_file_fd_len);
    if(!tx_storage_buffer)
    {
        goto err_handle;
    }

    /*********************************************************************************************************
                                    Reading from file /var/tmp/aesdsocketdata
    **********************************************************************************************************/
    // read() attempts to read up to count bytes from file descriptor fd
    // into the buffer starting at buf.
    // ssize_t read(int fd, void *buf, size_t count);
    // manpage: https://man7.org/linux/man-pages/man2/read.2.html
    int bytes_read = read(socket_file_fd, tx_storage_buffer, socket_file_fd_len);
    if(bytes_read == -1) // returns -1 on error else number of bytes read
    {
        printf("\nError: Failed read(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed read(). Error code: %d", errno);
        // return -1;
        goto err_handle;
    }

    printf("Data Len Transmitted: %d\n", socket_file_fd_len);
    for(i=0;i<socket_file_fd_len;i++)
    {
        printf("%c", *(tx_storage_buffer+i));
    }
    printf("\n"); // It seems line the system was buffering the printf so absense of new line was making it buffer and printing only the next time new line was met
    // Refer to https://stackoverflow.com/questions/39180642/why-does-printf-not-produce-any-output


    /*********************************************************************************************************
                                    Sending Data from server to client
    **********************************************************************************************************/
    // The system calls send(), sendto(), and sendmsg() are used to
    // transmit a message to another socket.
    // ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    // manpage: https://man7.org/linux/man-pages/man2/send.2.html
    int tx_data_len;
    tx_data_len = send(client_socket_fd, tx_storage_buffer, socket_file_fd_len, 0);
    if(tx_data_len == -1) // returns -1 on error else number of bytes sent
    {
        printf("\nError: Failed send(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed send(). Error code: %d", errno);
        // return -1;
        goto err_handle;
    }

    // The mutex object referenced by mutex shall be locked by a call to
    // pthread_mutex_lock() that returns zero or [EOWNERDEAD].  If the
    // mutex is already locked by another thread, the calling thread
    // shall block until the mutex becomes available.
    // int pthread_mutex_lock(pthread_mutex_t *mutex);
    //  manpage: https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html
    status = pthread_mutex_unlock(&mutex);
    if(status != 0) // returns non zero on error
    {
        printf("\nError: Failed pthread_mutex_unlock(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed pthread_mutex_unlock(). Error code: %d", errno);
        // return -1;
    }
#else

    int bytes_read;
    socket_file_fd_len = 10;

    tx_storage_buffer = (char*)malloc(socket_file_fd_len);
    if(!tx_storage_buffer)
    {
        goto err_handle;
    }

    while((bytes_read = read(socket_file_fd, tx_storage_buffer, socket_file_fd_len)) > 0) 
    {
        if( bytes_read == -1) 
        { 
            printf("\nError: Failed read(). Error code: %d\n", errno);
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Error: Failed read(). Error code: %d", errno);
            // return -1;
            goto err_handle;
        }
        if(send(client_socket_fd, tx_storage_buffer, bytes_read, 0) == -1) 
        {
            printf("\nError: Failed send(). Error code: %d\n", errno);
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Error: Failed send(). Error code: %d", errno);
            // return -1;
            goto err_handle;
        }
    }

#endif


err_handle:
    status = close(socket_file_fd);
    if(status != 0) // returns 0 if it succeeds else -1 on error
    {
        printf("\nError: Failed close() the socket_file_fd. Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed close() the server_socket_fd. Error code: %d", errno);
        // exit(-1);
    }    


    free(tx_storage_buffer);
    free(rx_storage_buffer);

    printf("Connection Closed with %s :: %d\n", ip, p);
    // Syslog the info into the syslog file in /var/log
    syslog(LOG_INFO, "Connection Closed with %s :: %d\n", ip, p);

    thread_arg->thread_complete = true;

    return NULL;
}


void ll_push(node_t **head_ref, node_t *node)
{
    node->next_node = *head_ref;
    *head_ref = node;
}

void clean_ll(node_t** head, int is_flag)
{
    if((!head) || !(*head))
    {
        return;
    }
    node_t *current_node = *head;
    node_t *prev_node = NULL;   
    while(current_node != NULL)
    {
        if ((current_node->data.thread_complete == true) || (1 == is_flag))
        {
            node_t *temp = current_node;
            if(current_node == *head)
            {
                current_node = current_node->next_node;
                *head = current_node;
            }
            else
            {
                current_node = current_node->next_node;
                prev_node->next_node = current_node;
            }
            if(temp)
            { 
                pthread_join(temp->data.thread_id, NULL);
                free(temp);
            }
        }
        else
        {
            prev_node = current_node;
            current_node = current_node->next_node;
        }
    }
}

int main (int argc, char** argv)
{
    // Appends aesdsocket.c to all the logs by default t is the program name
    // LOG_PERROR also logs error to stderr
    // Sets the facility to USER
    openlog("aesdsocket.c", LOG_PERROR, LOG_USER);

    // Initializing signals and their respective signal handlers
    sig_init();

    // // Initializing the timer
    // timer_init();

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
        printf("Daemon: Hello, World!\n");
    else
        printf("Normal: Hello, World!\n");

    /*********************************************************************************************************
                                                Server Setup
    **********************************************************************************************************/
    
    int status;
    int option_value = 1; // Used in setsockopt()

    // Creating an end point for communication (i.e. socket)
    // int socket(int domain, int type, int protocol);
    // manpage: https://man7.org/linux/man-pages/man2/socket.2.html 
    // Setting SOCK_NONBLOCK enables to use the accept4 or make the socket non block to check for kill_program flag
    int server_socket_fd;
    server_socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); 
    if (server_socket_fd == -1) // 0 on success and -1 for error
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
    struct addrinfo *server_info;
    char temp_str[20];

    memset(&hints, 0, sizeof(hints));    // Seting the struct to 0
    hints.ai_family = AF_INET;          // Only IPv4 support
    hints.ai_socktype = SOCK_STREAM;    // TCP stram sockets
    hints.ai_flags = AI_PASSIVE;        // Fill my IP for me

    // if((status = getaddrinfo(NULL, "9000", &hints, &server_info)) != 0)
    sprintf(temp_str, "%d", PORT_NUMBER);         // To convert the PORT_NUMBER into a string
    status = getaddrinfo(NULL, temp_str, &hints, &server_info);
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
    status = setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value));
    if(status == -1) // 0 on success and -1 for error
    {
        printf("\nError: Failed setsockopt(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed setsockopt(). Error code: %d", errno);
        freeaddrinfo(server_info);
        return -1;
    }

    // When a socket is created with socket(2), it exists in a name
    // space (address family) but has no address assigned to it.  bind()
    // assigns the address specified by addr to the socket referred to
    // by the file descriptor sockfd.  addrlen specifies the size, in
    // bytes, of the address structure pointed to by addr.
    // int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    // manpage: https://man7.org/linux/man-pages/man2/bind.2.html
    status = bind(server_socket_fd, server_info->ai_addr, server_info->ai_addrlen);
    if(status == -1) // 0 on success and -1 for error
    {
        printf("\nError: Failed bind(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed bind(). Error code: %d", errno);
        freeaddrinfo(server_info);
        return -1;
    }

    // The connection is now made and we no longer need the addrinfo so we 
    // will free server_info as it was malloced in the getaddrinfo() call
    // to avoid memory leaks.
    // void freeaddrinfo(struct addrinfo *ai);
    // manpage: https://man7.org/linux/man-pages/man3/freeaddrinfo.3p.html
    freeaddrinfo(server_info);

    /*********************************************************************************************************
                                                Server Listening
    **********************************************************************************************************/
    // listen() marks the socket referred to by sockfd as a passive
    // socket, that is, as a socket that will be used to accept incoming
    // connection requests using accept(2).
    // int listen(int sockfd, int backlog);
    // manpage: https://man7.org/linux/man-pages/man2/listen.2.html
    status = listen(server_socket_fd, 5); // Setting an arbitrary backlog value of 5
    if(status == -1) // 0 on success and -1 for error
    {
        printf("\nError: Failed listen(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed listen(). Error code: %d", errno);
        return -1;
    }

    // Note:
    // The socket here is listening to any IP of the machine at port 9000
    // Refer to https://stackoverflow.com/questions/4046616/sockets-how-to-find-out-what-port-and-address-im-assigned
    //      and https://stackoverflow.com/questions/212528/how-can-i-get-the-ip-address-of-a-linux-machine
    // There could be two connections from 2 IPs and this port eill respond to both
    // The below command will give the ports that are listening:
    // sudo lsof -i -P -n
    // It is noted that the port used here will not be tied to any specific IP and will display as *.9000
    // Basically meaning that it is any IP with port 9000

    if (run_as_daemon)
    status = daemon_init();
    if (status == -1)
    {
        printf("\nError: Failed init_daemon(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed init_daemon(). Error code: %d", errno);
        return -1;
    }

    printf("Listening.....\n");
    printf("\nListening on port: %d\n", PORT_NUMBER);
    // Syslog the info into the syslog file in /var/log
    syslog(LOG_INFO, "Listening on port: %d", PORT_NUMBER);


    int client_socket_fd;
    struct sockaddr_in client_sock_addr;
    socklen_t client_sock_addr_len = sizeof(client_sock_addr);

    node_t *head = NULL;

#ifndef USE_AESD_CHAR_DEVICE
    // Thread for timer handling
    pthread_t timer_thread_id;
    status = pthread_create(&timer_thread_id, NULL, print_cal, NULL);
    if (status != 0)
    {
        printf("\nError: Failed pthread_create(). Error code: %d\n", errno);
        // Syslog the error into the syslog file in /var/log
        syslog(LOG_ERR, "Error: Failed pthread_create(). Error code: %d", errno);
        return -1;
    }
    else
    {
        printf("Spawning timer thread with ID: %lu!\n", timer_thread_id);
    }
#endif

    /*********************************************************************************************************
                                        Server Accepting Connection Requests
    **********************************************************************************************************/
    while(!kill_program)
    {

        // The accept() system call is used with connection-based socket
        // types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first
        // connection request on the queue of pending connections for the
        // listening socket, sockfd, creates a new connected socket, and
        // returns a new file descriptor referring to that socket.  The
        // newly created socket is not in the listening state.  The original
        // socket sockfd is unaffected by this call.
        // If no pending connections are present on the queue, and the
        // socket is not marked as nonblocking, accept() blocks the caller
        // until a connection is present.  If the socket is marked
        // nonblocking and no pending connections are present on the queue,
        // accept() fails with the error EAGAIN or EWOULDBLOCK.
        // If flags is 0, then accept4() is the same as accept().
        // int accept(int sockfd, struct sockaddr *restrict addr,
        //            socklen_t *restrict addrlen);
        // int accept4(int sockfd, struct sockaddr *restrict addr,
        //            socklen_t *restrict addrlen, int flags);
        // manpage: https://man7.org/linux/man-pages/man2/accept.2.html
        client_socket_fd = accept(server_socket_fd, (struct sockaddr*) &client_sock_addr, &client_sock_addr_len);
        if(client_socket_fd == -1) // -1 for error and file descriptor for success
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                continue;
            }
            printf("\nError: Failed accept(). Error code: %d\n", errno);
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Error: Failed accept(). Error code: %d", errno);
        }
        
        node_t *new_node = (node_t*)malloc(sizeof(node_t));
        new_node->data.client_socket_fd = client_socket_fd;
        new_node->data.thread_complete = false;
        new_node->data.client_sock_addr = client_sock_addr;

        // Creating a thread to run threadfunc with parameters thread_param
        int status = pthread_create(&(new_node->data.thread_id), NULL, thread_func, &(new_node->data));
        if (status != 0)
        {
            printf("\nError: Failed pthread_create(). Error code: %d\n", errno);
            // Syslog the error into the syslog file in /var/log
            syslog(LOG_ERR, "Error: Failed pthread_create(). Error code: %d", errno);
            return -1;
        }
        else
        {
            printf("Spawning thread with ID: %lu!\n", (new_node->data.thread_id));
        }


        
        printf("Pushed to linkedlist\n");
        ll_push(&head, new_node);

        clean_ll(&head, 0);
    }

    clean_ll(&head, 1);

    struct clean_up_data clean_data;
    clean_data.server_soc_fd = server_socket_fd;

    

    program_kill_clean_up(clean_data);

    // printf("Was here!\n");

#ifndef USE_AESD_CHAR_DEVICE

    pthread_join(timer_thread_id, NULL);

#endif

    return 0;
}