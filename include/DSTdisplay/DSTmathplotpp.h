
#include <cstdio>   // for fflush, fileno
#include <fcntl.h>  // for O_WRONLY, O_CREAT, O_APPEND, open
#include <unistd.h> // for dup, dup2, close

namespace DSL
{
    namespace mathplot
    {
        class StderrRedirect
        {
                // Redirects stderr to a file (default /dev/null) within scope.
                // Used to silence gnuplot/matplot++ backend warnings while plotting.
                // Restores original stderr in destructor.
                int old_fd_ = -1;
                int new_fd_ = -1;
            public:
                explicit StderrRedirect(const char* path = "/dev/null")
                {
                    fflush(stderr);
                    old_fd_ = dup(fileno(stderr));
                    new_fd_ = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (new_fd_ >= 0) dup2(new_fd_, fileno(stderr));
                }
                ~StderrRedirect()
                {
                    fflush(stderr);
                    if (old_fd_ >= 0) dup2(old_fd_, fileno(stderr));
                    if (new_fd_ >= 0) close(new_fd_);
                    if (old_fd_ >= 0) close(old_fd_);
                }
        };
    }
}