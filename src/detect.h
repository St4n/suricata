#ifndef __DETECT_H__
#define __DETECT_H__

#include <stdint.h>

#include "detect-engine-proto.h"

#include "packet-queue.h"
#include "util-mpm.h"
#include "util-hash.h"
#include "util-hashlist.h"

#define COUNTER_DETECT_ALERTS 1

/* forward declarations for the structures from detect-engine-sigorder.h */
struct SCSigOrderFunc_;
struct SCSigSignatureWrapper_;

/*
 * DETECT ADDRESS
 */

/* a is ... than b */
enum {
    ADDRESS_ER = -1, /* error e.g. compare ipv4 and ipv6 */
    ADDRESS_LT,      /* smaller              [aaa] [bbb] */
    ADDRESS_LE,      /* smaller with overlap [aa[bab]bb] */
    ADDRESS_EQ,      /* exactly equal        [abababab]  */
    ADDRESS_ES,      /* within               [bb[aaa]bb] and [[abab]bbb] and [bbb[abab]] */
    ADDRESS_EB,      /* completely overlaps  [aa[bbb]aa] and [[baba]aaa] and [aaa[baba]] */
    ADDRESS_GE,      /* bigger with overlap  [bb[aba]aa] */
    ADDRESS_GT,      /* bigger               [bbb] [aaa] */
};

#define ADDRESS_FLAG_ANY 0x1
#define ADDRESS_FLAG_NOT 0x2

#define ADDRESS_GROUP_SIGGROUPHEAD_COPY  0x01
#define ADDRESS_GROUP_PORTS_COPY         0x02
#define ADDRESS_GROUP_PORTS_NOTUNIQ      0x04
#define ADDRESS_GROUP_HAVEPORT           0x08

typedef struct DetectAddressData_ {
    /* XXX convert to use a Address datatype to replace family, ip,ip2*/
    uint8_t family;
    uint32_t ip[4];
    uint32_t ip2[4];
    uint8_t flags;
} DetectAddressData;

typedef struct DetectAddressGroup_ {
    /* address data for this group */
    DetectAddressData *ad;

    /* XXX ptr to rules, or PortGroup or whatever */
    union {
        struct DetectAddressGroupsHead_ *dst_gh;
        struct DetectPort_ *port;
    };
    /* signatures that belong in this group */
    struct SigGroupHead_ *sh;
    uint8_t flags;

    /* double linked list */
    struct DetectAddressGroup_ *prev;
    struct DetectAddressGroup_ *next;

    uint32_t cnt;
} DetectAddressGroup;

/** Signature grouping head. Here 'any', ipv4 and ipv6 are split out */
typedef struct DetectAddressGroupsHead_ {
    DetectAddressGroup *any_head;
    DetectAddressGroup *ipv4_head;
    DetectAddressGroup *ipv6_head;
} DetectAddressGroupsHead;

/*
 * DETECT PORT
 */

/* a is ... than b */
enum {
    PORT_ER = -1, /* error e.g. compare ipv4 and ipv6 */
    PORT_LT,      /* smaller              [aaa] [bbb] */
    PORT_LE,      /* smaller with overlap [aa[bab]bb] */
    PORT_EQ,      /* exactly equal        [abababab]  */
    PORT_ES,      /* within               [bb[aaa]bb] and [[abab]bbb] and [bbb[abab]] */
    PORT_EB,      /* completely overlaps  [aa[bbb]aa] and [[baba]aaa] and [aaa[baba]] */
    PORT_GE,      /* bigger with overlap  [bb[aba]aa] */
    PORT_GT,      /* bigger               [bbb] [aaa] */
};

#define PORT_FLAG_ANY           0x01 /**< 'any' special port */
#define PORT_FLAG_NOT           0x02 /**< negated port */
#define PORT_SIGGROUPHEAD_COPY  0x04 /**< sgh is a ptr copy */
#define PORT_GROUP_PORTS_COPY   0x08 /**< dst_ph is a ptr copy */

/** \brief Port structure for detection engine */
typedef struct DetectPort_ {
    uint16_t port;
    uint16_t port2;

    /* signatures that belong in this group */
    struct SigGroupHead_ *sh;

    struct DetectPort_ *dst_ph;

    /* double linked list */
    union {
        struct DetectPort_ *prev;
        struct DetectPort_ *hnext; /* hash next */
    };
    struct DetectPort_ *next;

    uint32_t cnt;
    uint8_t flags;  /**< flags for this port */
} DetectPort;

/* Signature flags */
#define SIG_FLAG_RECURSIVE 0x0001   /**< recurive capturing enabled */
#define SIG_FLAG_SRC_ANY   0x0002   /**< source is any */
#define SIG_FLAG_DST_ANY   0x0004   /**< destination is any */
#define SIG_FLAG_SP_ANY    0x0008   /**< source port is any */
#define SIG_FLAG_DP_ANY    0x0010   /**< destination port is any */
#define SIG_FLAG_NOALERT   0x0020   /**< no alert flag is set */
#define SIG_FLAG_IPONLY    0x0040   /**< ip only signature */
#define SIG_FLAG_MPM       0x0080   /**< sig has mpm portion (content, uricontent, etc) */
#define SIG_FLAG_DEONLY    0x0100   /**< decode event only signature */
#define SIG_FLAG_PAYLOAD   0x0200   /**< signature is inspecting the packet payload */

/* Detection Engine flags */
#define DE_QUIET           0x01     /**< DE is quiet (esp for unittests) */

typedef struct DetectEngineIPOnlyThreadCtx_ {
    DetectAddressGroup *src, *dst;
    uint8_t *sig_match_array; /* bit array of sig nums */
    uint32_t sig_match_size;  /* size in bytes of the array */
} DetectEngineIPOnlyThreadCtx;

/** \brief Signature container */
typedef struct Signature_ {
    uint16_t flags;

    uint8_t rev;
    uint8_t prio;

    uint32_t gid; /**< generator id */
    uint32_t num; /**< signature number, internal id */
    uint32_t id;  /**< sid, set by the 'sid' rule keyword */
    char *msg;

    /** addresses, ports and proto this sig matches on */
    DetectAddressGroupsHead src, dst;
    DetectProto proto;
    DetectPort *sp, *dp;

    /** ptr to the SigMatch list */
    struct SigMatch_ *match;
    /** ptr to the next sig in the list */
    struct Signature_ *next;

    /** inline -- action */
    uint8_t action;
} Signature;

/** \brief IP only rules matching ctx.
 *  \todo a radix tree would be great here */
typedef struct DetectEngineIPOnlyCtx_ {
    /* lookup hashes */
    HashListTable *ht16_src, *ht16_dst;
    HashListTable *ht24_src, *ht24_dst;

    /* counters */
    uint32_t a_src_uniq16, a_src_total16;
    uint32_t a_dst_uniq16, a_dst_total16;
    uint32_t a_src_uniq24, a_src_total24;
    uint32_t a_dst_uniq24, a_dst_total24;

    uint32_t max_idx;

    uint8_t *sig_init_array; /* bit array of sig nums */
    uint32_t sig_init_size;  /* size in bytes of the array */

    /* number of sigs in this head */
    uint32_t sig_cnt;
    uint32_t *match_array;
} DetectEngineIPOnlyCtx;

typedef struct DetectEngineLookupFlow_ {
    DetectAddressGroupsHead *src_gh[256]; /* a head for each protocol */
    DetectAddressGroupsHead *tmp_gh[256];
} DetectEngineLookupFlow;

/* Flow status
 *
 * to server
 * to client
 */
#define FLOW_STATES 2
typedef struct DetectEngineLookupDsize_ {
    DetectEngineLookupFlow flow_gh[FLOW_STATES];
} DetectEngineLookupDsize;

/* Dsize states
 * <= 100
 * >100
 */
#define DSIZE_STATES 2

/** \brief main detection engine ctx */
typedef struct DetectEngineCtx_ {
    uint8_t flags;

    Signature *sig_list;
    uint32_t sig_cnt;

    Signature **sig_array;
    uint32_t sig_array_size; /* size in bytes */
    uint32_t sig_array_len;  /* size in array members */

    uint32_t signum;

    /* used by the signature ordering module */
    struct SCSigOrderFunc_ *sc_sig_order_funcs;
    struct SCSigSignatureWrapper_ *sc_sig_sig_wrapper;

    /* main sigs */
    DetectEngineLookupDsize dsize_gh[DSIZE_STATES];

    uint32_t mpm_unique, mpm_reuse, mpm_none,
              mpm_uri_unique, mpm_uri_reuse, mpm_uri_none;
    uint32_t gh_unique, gh_reuse;

    uint32_t mpm_max_patcnt, mpm_min_patcnt, mpm_tot_patcnt,
              mpm_uri_max_patcnt, mpm_uri_min_patcnt, mpm_uri_tot_patcnt;

    /* content and uricontent vars */
    uint32_t content_max_id;
    uint32_t uricontent_max_id;

    /* init phase vars */
    HashListTable *sgh_hash_table;

    HashListTable *sgh_mpm_hash_table;
    HashListTable *sgh_mpm_uri_hash_table;

    HashListTable *sgh_sport_hash_table;
    HashListTable *sgh_dport_hash_table;

    HashListTable *sport_hash_table;
    HashListTable *dport_hash_table;

    HashListTable *variable_names;
    uint16_t variable_names_idx;

    /* memory counters */
    uint32_t mpm_memory_size;

    DetectEngineIPOnlyCtx io_ctx;

    uint16_t mpm_matcher; /**< mpm matcher this ctx uses */
} DetectEngineCtx;

/**
  * Detection engine thread data.
  */
typedef struct DetectionEngineThreadCtx_ {
    /* detection engine variables */
    uint8_t *pkt_ptr; /* ptr to the current position in the pkt */
    uint16_t pkt_off;
    uint8_t pkt_cnt;

    char de_checking_distancewithin;

    /* http_uri stuff for uricontent */
    char de_have_httpuri;

    /** pointer to the current mpm ctx that is stored
     *  in a rule group head -- can be either a content
     *  or uricontent ctx. */
    MpmThreadCtx mtc; /**< thread ctx for the mpm */
    MpmThreadCtx mtcu;
    struct SigGroupHead_ *sgh;
    PatternMatcherQueue pmq;

    /* counters */
    uint32_t pkts;
    uint32_t pkts_scanned;
    uint32_t pkts_searched;
    uint32_t pkts_scanned1;
    uint32_t pkts_searched1;
    uint32_t pkts_scanned2;
    uint32_t pkts_searched2;
    uint32_t pkts_scanned3;
    uint32_t pkts_searched3;
    uint32_t pkts_scanned4;
    uint32_t pkts_searched4;

    uint32_t uris;
    uint32_t pkts_uri_scanned;
    uint32_t pkts_uri_searched;
    uint32_t pkts_uri_scanned1;
    uint32_t pkts_uri_searched1;
    uint32_t pkts_uri_scanned2;
    uint32_t pkts_uri_searched2;
    uint32_t pkts_uri_scanned3;
    uint32_t pkts_uri_searched3;
    uint32_t pkts_uri_scanned4;
    uint32_t pkts_uri_searched4;

    /** id for alert counter */
    uint16_t counter_alerts;

    /** ip only rules ctx */
    DetectEngineIPOnlyThreadCtx io_ctx;

    DetectEngineCtx *de_ctx;
} DetectEngineThreadCtx;

/** \brief a single match condition for a signature */
typedef struct SigMatch_ {
    uint8_t type; /**< match type */
    void *ctx; /**< plugin specific data */
    struct SigMatch_ *next;
    struct SigMatch_ *prev;
} SigMatch;

/** \brief element in sigmatch type table. */
typedef struct SigTableElmt_ {
    int (*Match)(ThreadVars *, DetectEngineThreadCtx *, Packet *, Signature *, SigMatch *);
    int (*Setup)(DetectEngineCtx *, Signature *, SigMatch *, char *);
    void (*Free)(void *);
    void (*RegisterTests)(void);

    uint8_t flags;
    char *name;
} SigTableElmt;

#define SIG_GROUP_HAVECONTENT          0x01
#define SIG_GROUP_HAVEURICONTENT       0x02
#define SIG_GROUP_HEAD_MPM_COPY        0x04
#define SIG_GROUP_HEAD_MPM_URI_COPY    0x08
#define SIG_GROUP_HEAD_FREE            0x10

/** \brief head of the list of containers. */
typedef struct SigGroupHead_ {
    uint8_t flags;

    /* pattern matcher instance */
    MpmCtx *mpm_ctx;      /* search */
    uint16_t mpm_content_maxlen;
    MpmCtx *mpm_uri_ctx;
    uint16_t mpm_uricontent_maxlen;

    /* number of sigs in this head */
    uint32_t sig_cnt;

    uint8_t *sig_array; /* bit array of sig nums */
    uint32_t sig_size; /* size in bytes */

    /* array with sig nums... size is sig_cnt * sizeof(uint32_t) */
    uint32_t *match_array;

    /* list of content containers
     * XXX move into a separate data struct
     * with only a ptr to it. Saves some memory
     * after initialization
     */
    uint32_t *content_array;
    uint32_t content_size;
    uint32_t *uri_content_array;
    uint32_t uri_content_size;

    /* port ptr */
    struct DetectPort_ *port;

    uint16_t mpm_len1;
    uint16_t mpm_len2;
    uint16_t mpm_len3;
    uint16_t mpm_len4; /* 4+ */
} SigGroupHead;

/** sigmatch has no options, so the parser shouldn't expect any */
#define SIGMATCH_NOOPT          0x01
/** sigmatch is compatible with a ip only rule */
#define SIGMATCH_IPONLY_COMPAT  0x02
/** sigmatch is compatible with a decode event only rule */
#define SIGMATCH_DEONLY_COMPAT  0x04
/**< Flag to indicate that the signature inspects the packet payload */
#define SIGMATCH_PAYLOAD        0x08

/** Remember to add the options in SignatureIsIPOnly() at detect.c otherwise it wont be part of a signature group */

enum {
    DETECT_SID,
    DETECT_PRIORITY,
    DETECT_REV,
    DETECT_CLASSTYPE,
    DETECT_THRESHOLD,
    DETECT_METADATA,
    DETECT_REFERENCE,
    DETECT_MSG,
    DETECT_CONTENT,    /* 8 */
    DETECT_URICONTENT, /* 9 */
    DETECT_PCRE,       /* 10 */
    DETECT_ACK,
    DETECT_SEQ,
    DETECT_DEPTH,
    DETECT_DISTANCE,
    DETECT_WITHIN,
    DETECT_OFFSET,
    DETECT_NOCASE,
    DETECT_RECURSIVE,
    DETECT_RAWBYTES,
    DETECT_BYTETEST,
    DETECT_BYTEJUMP,
    DETECT_SAMEIP,
    DETECT_IPPROTO,
    DETECT_FLOW,
    DETECT_WINDOW,
    DETECT_ISDATAAT,
    DETECT_ID,
    DETECT_DSIZE,
    DETECT_FLOWVAR,
    DETECT_PKTVAR,
    DETECT_NOALERT,
    DETECT_FLOWBITS,
    DETECT_IPV4_CSUM,
    DETECT_TCPV4_CSUM,
    DETECT_TCPV6_CSUM,
    DETECT_UDPV4_CSUM,
    DETECT_UDPV6_CSUM,
    DETECT_ICMPV4_CSUM,
    DETECT_ICMPV6_CSUM,
    DETECT_STREAM_SIZE,

    DETECT_ADDRESS,
    DETECT_PROTO,
    DETECT_PORT,
    DETECT_DECODE_EVENT,
    DETECT_IPOPTS,
    DETECT_FLAGS,
    DETECT_FRAGBITS,
    DETECT_GID,

    /* make sure this stays last */
    DETECT_TBLSIZE,
};

/* Table with all SigMatch registrations */
SigTableElmt sigmatch_table[DETECT_TBLSIZE];

/* detection api */
SigMatch *SigMatchAlloc(void);
void SigMatchAppend(Signature *, SigMatch *, SigMatch *);
void SigCleanSignatures(DetectEngineCtx *);

void SigTableRegisterTests(void);
void SigRegisterTests(void);
void TmModuleDetectRegister (void);

int SigGroupBuild(DetectEngineCtx *);
int SigGroupCleanup();

int PacketAlertAppend(Packet *, uint32_t, uint32_t, uint8_t, uint8_t, char *);

int SigLoadSignatures (DetectEngineCtx *, char *);
void SigTableSetup(void);
int PacketAlertCheck(Packet *p, uint32_t sid);
int SigMatchSignatures(ThreadVars *th_v, DetectEngineCtx *de_ctx, DetectEngineThreadCtx *det_ctx, Packet *p);

int PacketAlertCheck(Packet *p, uint32_t sid);
int SigMatchSignatures(ThreadVars *th_v, DetectEngineCtx *de_ctx, DetectEngineThreadCtx *det_ctx, Packet *p);
int SignatureIsIPOnly(DetectEngineCtx *de_ctx, Signature *s);

#endif /* __DETECT_H__ */

