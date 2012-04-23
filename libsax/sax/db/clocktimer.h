#include <sys/time.h>

class ClockTimer
{
public:
    /// @brief remembers start time during construction
    ClockTimer()
    : start_()
    {
        gettimeofday(&start_, 0);
    }

    /// @brief resets start time to current time.
    void restart()
    {
        gettimeofday(&start_, 0);
    }
    
     /// @brief returns elapsed seconds since remembered start time.
     /// @return elapsed time in seconds, some platform can returns fraction
     /// representing more precise time.
    double elapsed() const
    {
        timeval now;
        gettimeofday(&now, 0);

        double seconds = now.tv_sec - start_.tv_sec;
        seconds += (now.tv_usec - start_.tv_usec) / 1000000.0;

        return seconds;
    }

private:
    timeval start_; /**< remembers start time */
};
