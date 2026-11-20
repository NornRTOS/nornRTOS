/**
 * @file store_config.h
 * nornStore options of the device example (nornstore.mk finds this file next to the
 * application). The validate hook keeps `system.loglvl` within the classes hal/log.h
 * knows; `config set system.loglvl 9` is refused with "value".
 */
#ifndef STORE_CONFIG_INCLUDED
#define STORE_CONFIG_INCLUDED

#define STORE_CFG_VALIDATE_HOOK     deviceValidate

#endif
