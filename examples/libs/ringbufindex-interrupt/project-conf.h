#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/*---------------------------------------------------------------------------*/
/*
 * Length of the message queue.
 */
#define QUEUE_LEN 128

/*---------------------------------------------------------------------------*/

/*
 * Number of messages that the normal producer puts.
 */
#define NORMAL_PUT_NUM 0

/*
 * Number of messages that the interrupt producer puts.
 */
#define INTERRUPT_PUT_NUM 3000

/*
 * Number of messages that the normal consumer gets.
 */
#define NORMAL_GET_NUM 3000

/*
 * Number of messages that the interrupt consumer gets.
 */
#define INTERRUPT_GET_NUM ((NORMAL_PUT_NUM) + (INTERRUPT_PUT_NUM) - (NORMAL_GET_NUM))

/*
 * If 1, it uses the "peek" API for put. If 0, it uses the "atomic" API.
 */
#define PUT_USE_PEEK 1

/*
 * If 1, it uses the "peek" API for get. If 0, it uses the "atomic" API.
 */
#define GET_USE_PEEK 1

/*---------------------------------------------------------------------------*/

/*
 * Time interval of interrupts
 */
#define INTERRUPT_RTIMER_INTERVAL (US_TO_RTIMERTICKS(10))

/*
 * Extra delay for moving one message.
 *
 * The higher this value is, the more often that the queue becomes
 * inconsistent (if appropriate API is not used)
 */
#define MOVE_WAIT_COUNT 100

#endif /* PROJECT_CONF_H_ */
