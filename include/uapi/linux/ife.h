#ifndef __UAPI_IFE_H
#define __UAPI_IFE_H

#define IFE_METAHDRLEN 2

#define IFE_META_SKBMARK 1
#define IFE_META_HASHID 2
#define IFE_META_PRIO 3
#define IFE_META_QMAP 4
#define IFE_META_TCINDEX 5
#define IFE_META_IFINDEX 6
#define IFE_META_ORIGSIZE 7
#define IFE_META_SIZE 8

/*Can be overridden at runtime by module option*/
#define __IFE_META_MAX 6
#define IFE_META_MAX (__IFE_META_MAX - 1)

#endif
