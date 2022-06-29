# Orchestra

## Overview

Orchestra is an autonomous scheduling solution for TSCH, where nodes maintain
their own schedule solely based on their local RPL state. There is no centralized
scheduler nor negociatoin with neighbors, i.e. no traffic overhead. The default
Orchestra rules can be used out-of-box in any RPL network, reducing contention
to a low level. Orchestra is described and evaluated in
[*Orchestra: Robust Mesh Networks Through Autonomously Scheduled TSCH*](http://www.simonduquennoy.net/papers/duquennoy15orchestra.pdf), ACM SenSys'15.

## Requirements

Orchestra requires a system running TSCH and RPL.

## Getting Started

To include Orchestra in your application, all you need to do is to include its module, by adding the following line to your makefile:
```
MODULES += os/services/orchestra
```

## Configuration

Orchestra comes with a number of pre-installed rules, `orchestra-rule-*.c`.
You can define your own by using any of these as a template.
A default Orchestra configuration is described in `orchestra-conf.h`, define your own
`ORCHESTRA_CONF_*` macros to override modify the rule set and change rules configuration.
