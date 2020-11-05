#ifndef __GPL_UTILS_H__
#define __GPL_UTILS_H__

/* all this stuff is stolen from, or based on code from, the Linux kernel */
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#endif
