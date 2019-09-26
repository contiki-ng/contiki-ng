/*
 * When memb.h is "included", contiki.h is also loaded as a result
 * since it is a dependency of cc.h, which memb.h needs for
 * CC_CONCAT() macro.
 *
 * This file serves as a dummy contiki.h to make it possible to
 * compile a test file having "#include <lib/memb.h>" in it.
 */
