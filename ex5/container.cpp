#include <iostream>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <fstream>

// Constants
#define STACK_SIZE  8192
#define NUM_ARG_NOT_EXTRA  5
#define ERROR 1
#define FAILURE -1
#define PREMISSION 0755
#define ONE "1"
#define BUFFER_SIZE 256

//Path constants
#define DIR_FS  "/sys/fs"
#define DIR_C_GROUP  "/sys/fs/cgroup"
#define DIR_PIDS  "/sys/fs/cgroup/pids"
#define C_GROUPS_PROCS  "/sys/fs/cgroup/pids/cgroup.procs"
#define C_PIDS_MAX "/sys/fs/cgroup/pids/pids.max"
#define C_NOT_RELEASE "/sys/fs/cgroup/pids/notify_on_release"

//Error massage constants
#define ERR_MEMORY_ALLOC "failure in the Allocation function"
#define ERR_HOST_NAME "failure in set host name function"
#define ERR_C_ROOT "failure in C H Root function"
#define ERR_C_DIR "failure in C H Dir function"
#define ERR_MK_DIR "failure in the mkdir function"
#define ERR_OPEN_FILE "failure in the open file function"
#define ERR_MOUNT "failure in the mount function"
#define ERR_UMOUNT "failure in the umount function"
#define ERR_EXECVP "failure in the execvp function"
#define ERR_REMOVE "failure in the remove function"
#define ERR_RMDIR "failure in the rmdir function"


// typedef DECLARATION
using namespace std;

// Private Library Fields
struct argument {
    const char *name;
    const char *newFsDirectory;
    const char *numProses;
    char *pathToProgram;
    char **extraArg;
};


enum FileKind {
    FLE, DIR
};

// ----- Helper Functions --------
/**
 *
 * @param problem - The string of the massage that need to be printed.
 */
void system_call_failure_printer(const std::string &problem) {
    std::cerr << "system error: " << problem << std::endl;
}


/**
 * Open a directory.
 * @param path - Contain the directory name and place of creation.
 */
void CreatDirectory(const char *path) {
    if (mkdir(path, PREMISSION) == FAILURE) {
        system_call_failure_printer(ERR_MK_DIR);
        exit(ERROR);
    }
}

/**
 * Write into a file.
 * @param path - the path to the file that needs to write in.
 * @param value - the value to write in to the file.
 */
void WriteToFile(const char *path, const char *value) {
    ofstream myFile; // An object we use for using files

    //Opening the file
    myFile.open(path);
    if (!myFile.is_open()) {
        system_call_failure_printer(ERR_OPEN_FILE);
        exit(ERROR);
    }

    myFile << value;    //Writing in to the file.
    myFile.close();     // Closing
}

/**
 * The given function for our container maker.
 * Set all the container values and setting.
 */
int child(void *arg) {

    auto *arguments = (argument *) arg;

    //Change the hostname
    if (sethostname(arguments->name, strlen(arguments->name)) == FAILURE) {
        system_call_failure_printer(ERR_HOST_NAME);
        exit(ERROR);
    }

    //Change the root directory
    if (chroot(arguments->newFsDirectory) == FAILURE) {
        system_call_failure_printer(ERR_C_ROOT);
        exit(ERROR);
    }

    //changes the current working directory of the calling process to the directory specified in path.
    if (chdir("/") == FAILURE) {
        system_call_failure_printer(ERR_C_DIR);
        exit(ERROR);
    }

    //Creates the request directories.
    CreatDirectory(DIR_FS); // Creating "fs" inside "sys"
    CreatDirectory(DIR_C_GROUP); // Creating "cgroup" inside "fs"
    CreatDirectory(DIR_PIDS); // Creating "pids"  inside "cgroup"

    //Creating the file "procs" inside the "pids" directory and writing the value 1 in it.
    //From within the container the process ID view should only show the processes running within the container
    // with the first process in the container being process 1
    WriteToFile(C_GROUPS_PROCS, ONE);

    //Creating the file "max" inside the "pids" directory and writing the max presses we can run in it.
    //Limiting the number of processes that can run within the container by generating the appropriate cgroups.
    WriteToFile(C_PIDS_MAX, arguments->numProses);

    //Creating the file "notify_on_release" inside the "pids" directory and writing value 1.
    WriteToFile(C_NOT_RELEASE, ONE); //TODO // SEE IF THIS IS THE RIGHT PLACE TO DO THIS.

    //Setting the permission (the premission number is given as a constant) to new files
    chmod(C_GROUPS_PROCS, PREMISSION);
    chmod(C_PIDS_MAX, PREMISSION);
    chmod(C_NOT_RELEASE, PREMISSION);

    //Mount the new procfs.
    //The mount command serves to attach the filesystem found on some device to the big file tree.
    if (mount("proc", "/proc", "proc", 0, nullptr) == FAILURE) {
        system_call_failure_printer(ERR_MOUNT);
        exit(ERROR);
    }

    //Run the terminal/new program
    char *_args[] = {arguments->pathToProgram, *(arguments->extraArg), nullptr};
    if (execvp(arguments->pathToProgram, _args) == FAILURE) {
        system_call_failure_printer(ERR_EXECVP);
        std::cout << std::strerror(errno) << std::endl;
        exit(ERROR);
    }
    return 0;
}

/**
 * An helper function for the ClearDirectories function.
 * @param pathOrigin the address of the directory we are in.
 * @param pateExtra the address of the file we want to delete from the directory we ae in.
 * @param fileKind do we want to delete a directory or a file.
 */
void ClearDirectoriesHelper(const char *pathOrigin, const char *pateExtra, FileKind fileKind) {
    string path = ((string) pathOrigin + (string) pateExtra);
    switch (fileKind) {
        case FLE:
            if (remove(path.c_str()) == FAILURE) {
                system_call_failure_printer(ERR_REMOVE);
                exit(ERROR);
            }
            break;
        case DIR:
            if (rmdir(path.c_str()) == FAILURE) {
                system_call_failure_printer(ERR_RMDIR);
                exit(ERROR);
            }
            break;
    }
}

/**
 * Clears and delete files/directories.
 * @param pathOrigin the address of the directory we are in.
 */
void ClearDirectories(const char *pathOrigin) {
    ClearDirectoriesHelper(pathOrigin, C_NOT_RELEASE, FLE);
    ClearDirectoriesHelper(pathOrigin, C_PIDS_MAX, FLE);
    ClearDirectoriesHelper(pathOrigin, C_GROUPS_PROCS, FLE);
    ClearDirectoriesHelper(pathOrigin, DIR_PIDS, DIR);
    ClearDirectoriesHelper(pathOrigin, DIR_C_GROUP, DIR);
    ClearDirectoriesHelper(pathOrigin, DIR_FS, DIR);
}

int main(int argc, char *argv[]) {
    //Creating a memory segment for the new process stack.
    char *stack = new(std::nothrow) char[STACK_SIZE];
    if (!stack) {
        system_call_failure_printer(ERR_MEMORY_ALLOC);
        exit(ERROR);
    }


    int numArgOfFunc = argc - NUM_ARG_NOT_EXTRA; //Calculating number of arguments for the container program to have.
    char **extraArgsVec = new(std::nothrow) char *[numArgOfFunc]; //An array of char* for the extra arguments.
    if (!extraArgsVec) {
        system_call_failure_printer(ERR_MEMORY_ALLOC);
        exit(ERROR);
    }

    //Inserting the extra arguments to the array.
    for (int i = 0; i < numArgOfFunc; ++i) {
        char *curr = argv[NUM_ARG_NOT_EXTRA + i];
        extraArgsVec[i] = curr;
    }

    //A Data structure we are using to store the argv arguments
    auto *newArgs = new(std::nothrow) argument{argv[1], argv[2], argv[3],
                                               argv[4], extraArgsVec};
    if (!newArgs) {
        system_call_failure_printer(ERR_MEMORY_ALLOC);
        exit(ERROR);
    }

    //These system calls create a new ("child") process.
    clone(child, stack + STACK_SIZE, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, newArgs);

    //CallING wait() from the process creating the container, so as not to finish before we are done with the container.
    wait(nullptr);

    //The umount command detaches the mentioned filesystem(s) from the file hierarchy.
    string path = ((string) argv[2] + (string) "/proc");
    if (umount(path.c_str()) == FAILURE) {
        system_call_failure_printer(ERR_UMOUNT);
        exit(1);
    }

    //Deleting the files and directories we made.
    ClearDirectories(argv[2]);

    //Frees the memory we used.
    free(stack);
    free(extraArgsVec);
    return 0;
}
