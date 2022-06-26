#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

char **environ;

static int _envc = 0;
static int _envc_len = 0;

int putenv(char *string)
{
    char *value = strchr(string, '=');
    if (!value)
    {
        ERRNO_RET(-EINVAL, -1);
    }

    int name_len = value - string;
    char name[name_len + 1];
    strncpy(name, string, name_len);
    if (getenv(name))
    {
        return -1;
    }

    if (!_envc_len)
    {
        _envc_len = 16;
        environ = malloc(sizeof(char *) * _envc_len);
    }

    environ[_envc++] = strdup(string);
    environ[_envc] = NULL;
    return 0;
}

char *getenv(const char *name)
{
    if (!environ)
        return NULL;
    int len = 0;
    while (name[len])
    {
        if (name[len] == '=')
        {
            errno = EINVAL;
            return NULL;
        }
        len++;
    }

    int i = 0;
    while (environ[i])
    {
        if (!strncmp(environ[i], name, len))
        {
            return environ[i] + len + 1;
        }
        i++;
    }

    return NULL;
}

int setenv(const char *envname, const char *envval, int overwrite)
{
    if (!envname || strchr(envname, '='))
        ERRNO_RET(-EINVAL, -1);
    char *env = getenv(envname);
    if (env)
    {
        if (overwrite)
        {
            int i = 0;
            int len = strlen(envname);
            while (environ[i])
            {
                if (!strncmp(environ[i], envname, len))
                {
                    char *e = malloc(len + strlen(envval) + 2);
                    sprintf(e, "%s=%s", envname, envval);
                    free(environ[i]);
                    environ[i] = e;
                    return 0;
                }
                i++;
            }
            return 0;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (_envc >= _envc_len)
        {
            _envc_len += 16;
            environ = realloc(environ, sizeof(char *) * _envc_len);
        }
        char *e = malloc(strlen(envname) + strlen(envval) + 2);
        sprintf(e, "%s=%s", envname, envval);

        environ[_envc] = e;
        _envc++;
        environ[_envc] = NULL;
        return 0;
    }
    return 0;
}

int unsetenv(const char *name)
{
    if (!name || strchr(name, '='))
        ERRNO_RET(-EINVAL, -1);
    if (!getenv(name))
    {
        return 0;
    }

    int i = 0;
    int len = strlen(name);
    while (environ[i])
    {
        if (!strncmp(environ[i], name, len))
        {
            free(environ[i]);
            for (int j = i; environ[j] != NULL; j++)
            {
                environ[j] = environ[j + 1];
            }
            _envc--;
            return 0;
        }
        i++;
    }
    assert(0);
    return -1;
}
