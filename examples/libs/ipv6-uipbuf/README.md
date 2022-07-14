# libs/ipv6-uipbuf

This example is meant to showcase the capabilities of uipbuf. It currently
focuses only on UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS, which lets an application
set a custom max number of MAC transmissions. Optionally, this information
can be passed on over multiple hops, so that the attribute applies along
the full path. This requires setting UIP_CONF_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS.
