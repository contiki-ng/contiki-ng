/* node.c starts the MAC explicitly via NETSTACK_MAC.on(). */
#define TSCH_CONF_AUTOSTART 0

/* Keep RPL parent changes and TSCH association visible, so that a failing
 * run in CI carries enough context to diagnose it. Per-slot TSCH logging
 * stays off: it is very verbose and is lossy under load. */
#define LOG_CONF_LEVEL_RPL              LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC              LOG_LEVEL_INFO
