MODULES += os/services/slip-cmd

PROJECT_SOURCEFILES += border-router-cmds.c tun-bridge.c httpd-simple.c
PROJECT_SOURCEFILES += slip-config.c slip-dev.c border-router-mac.c

MAKE_MAC = MAKE_MAC_OTHER
MAKE_NET = MAKE_NET_IPV6

connect-router:	border-router.native
	sudo ./border-router.native fd00::1/64
