#ifndef MSGSTRUCT_H
#define MSGSTRUCT_H

//message type start
#define ADD_REQ 0
#define ADD_RES 99
#define ATH_REQ 1
#define ATH_RES 2
#define CRT_TP_REQ 3
#define CRT_TP_RES 4
#define SUB_TP_REQ 5
#define SUB_TP_RES 6
#define GET_TP_UP_REQ 7
#define GET_TP_UP_RES 8
#define PUB_TP_REQ 9
#define PUB_TP_RES 10
#define RM_TP_REQ 11
#define RM_TP_RES 12
#define DTH_REQ 13
#define DTH_RES 14
#define RM_REQ 15
#define RM_RES 16
//message type end

#define ACK 0
#define NACK 99


//topic type
#define TP_LED 0    
#define TP_TEMP 101
#define TP_POWER_SWITCH 102

/**
    This code is for a sensor module connected to a gateway.
    It uses EEPROM memory to save & retrive allocated writing pipe addresses,
    whereas the reading pipe address are fixed.

    Note : Here I used this node as LED on/off actuator.

**/

// message structures start

struct __attribute__((packed)) add_node
{
  uint16_t nid;
  uint64_t wpipe;
};

struct __attribute__((packed)) attach_request
{
  uint16_t nid;
  uint64_t rpipe;
  uint64_t wpipe;
};

struct __attribute__((packed)) attach_respond
{
  uint16_t res;
  uint16_t nid;
};

typedef struct __attribute__((packed)) topic_t
{
  uint16_t tid;
  uint8_t type ;
} Topic;

struct __attribute__((packed)) create_topic_req
{
  uint16_t nid;
  Topic t;
};

struct __attribute__((packed)) create_topic_res
{
  uint16_t nid;
  Topic t;
  uint8_t res;
};
struct __attribute__((packed)) subscribe_topic_req
{
  uint16_t nid;
  Topic t;
};

struct __attribute__((packed)) subscribe_topic_res
{
  uint16_t nid;
  Topic t;
};

struct __attribute__((packed)) publish_topic_req
{
  uint16_t tid;
  uint32_t tdata;
};

struct __attribute__((packed)) publish_topic_res
{
  uint16_t tid;
  uint8_t res;
};


struct __attribute__((packed)) get_topic_update_req{
  uint16_t tid;
  uint16_t nid;
};

struct __attribute__((packed)) get_topic_update_res
{
  uint16_t tid;
  uint32_t tdata;
};


typedef struct __attribute__((packed)) message_t
{
  uint16_t nid;
  uint8_t type;
  union {
    struct add_node add_req;
    struct attach_request ath_req;
    struct attach_respond ath_res;
    struct subscribe_topic_req sub_tp_req;
    struct subscribe_topic_res sub_tp_res;
    struct get_topic_update_req get_tp_up_req;
    struct get_topic_update_res get_tp_up_res;
    struct create_topic_req crt_tp_req;
    struct create_topic_res crt_tp_res;
    struct publish_topic_req pub_tp_req;
    struct publish_topic_res pub_tp_res;

  } data;
} message;

//message structure end
#endif