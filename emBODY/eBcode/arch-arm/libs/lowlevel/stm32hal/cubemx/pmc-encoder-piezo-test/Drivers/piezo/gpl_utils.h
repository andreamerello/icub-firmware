#ifndef __GPL_UTILS_H__
#define __GPL_UTILS_H__

/* all this stuff is stolen from, or based on code from, the Linux kernel */
#define __ACCESS_ONCE(x) ({		 \
	typeof(x) __var = (typeof(x)) 0; \
	(volatile typeof(x) *)&(x); })
#define ACCESS_ONCE(x) (*__ACCESS_ONCE(x))
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

int fls(unsigned int x);

#endif
