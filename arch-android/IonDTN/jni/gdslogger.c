#include <android/log.h>

extern void	writeMemoToIonLog(char *text);

void	logToLogcat(char *text)
{
    if (text)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "ION", "%s\n", text);
    }
}

// Jay Gao remove reference to writeMemoToLogCatAndFile
//void	writeMemoToLogCatAndFile(char *text)
//{
//	logToLogcat(text);
//	writeMemoToIonLog(text);
//}