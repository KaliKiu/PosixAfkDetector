# PosixAfkDetector
PosixAfkDetector provides a simple C library for tracking user activity states. It monitors input events to determine when a user has gone AFK and when they've returned, with configurable timeout thresholds.
## API Functions
```c
// Start the AFK detection system
void afk_start();

// Set the AFK timeout in seconds
void afk_set_timeout(int seconds);

// Check if the user is currently AFK
int afk_status();

// Notify the system that user input was received
void afk_input();

// Reset the AFK timer and status
void afk_reset();
```


Still work in progress, adding additional API Functions such as: 
\n-Callback Function
\n-Compatibility with multi-process environments, including inter-process communication and piping.