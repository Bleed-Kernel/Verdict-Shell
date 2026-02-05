#include <syscalls/time.h>
#include <stdio.h>

const char* weekdays[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const char* months[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};

int day_of_week(int day, int month, int year) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int K = year % 100;
    int J = year / 100;
    int h = (day + 13*(month+1)/5 + K + K/4 + J/4 + 5*J) % 7;
    return ((h + 6) % 7);
}

const char* ordinal(int day) {
    if (day >= 11 && day <= 13) return "th";
    switch (day % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

void print_time(time_t t) {
    int wday = day_of_week(t.day, t.mon, t.year);
    printf("%s %d%s %s %d %02d:%02d:%02d\n",
        weekdays[wday],
        t.day, ordinal(t.day),
        months[t.mon - 1],
        t.year,
        t.hour, t.min, t.sec
    );
}