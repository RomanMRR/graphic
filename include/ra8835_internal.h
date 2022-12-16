/**
 * @defgroup    drivers_ra8835 RA8835 graphic LCD driver
 * @ingroup     drivers_actuators
 * @brief       Driver for the RA8835 graphic LCD driver
 *
 * @note        The driver currently supports direct addressing, no I2C
 *
 * @{
 *
 * @file
 * @brief       Internal config and parameters RA8835 graphic LCD driver
 *
 * @author      Alexander Podshivalov <a_podshivalov@mail.ru>
 */
#ifndef RA8835_INTERNAL_H
#define RA8835_INTERNAL_H

#include <stdint.h>

#include "periph/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Defined at ra8835_font.c */
extern const uint8_t ra8835_font[];

/**
 * @name    RA8835 LCD commands
 * @{
 */
#define RA8835_SYSTEM_SET              (0x40)
#define RA8835_SLEEP_IN                (0x53)
#define RA8835_DISPLAY_ON              (0x59)
#define RA8835_DISPLAY_OFF             (0x58)
#define RA8835_SCROLL                  (0x44)
#define RA8835_CSRFORM                 (0x5D)
#define RA8835_CGRAM_ADR               (0x5C)
#define RA8835_CSRDIR_RIGHT            (0x4C)
#define RA8835_CSRDIR_LEFT             (0x4D)
#define RA8835_CSRDIR_UP               (0x4E)
#define RA8835_CSRDIR_DOWN             (0x4F)
#define RA8835_HDOT_SCR                (0x5A)
#define RA8835_OVLAY                   (0x5B)
#define RA8835_CSRW                    (0x46)
#define RA8835_CSRR                    (0x47)
#define RA8835_MWRITE                  (0x42)
#define RA8835_MREAD                   (0x43)
/** @} */

/**
 * @name    RA8835 LCD timings
 * @{
 */
#define RA8835_RESET_PULSE             (2U)
/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* RA8835_INTERNAL_H */
/** @} */