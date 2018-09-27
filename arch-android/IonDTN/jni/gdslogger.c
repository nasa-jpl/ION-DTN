#include <android/log.h>

void	logToLogcat(char *text)
{
    if (text)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "ION", "%s\n", text);
    }
}
