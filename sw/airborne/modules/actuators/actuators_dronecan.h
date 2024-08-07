/*
 * Copyright (C) 2021 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef ACTUATORS_DRONECAN_H
#define ACTUATORS_DRONECAN_H

#include "modules/dronecan/dronecan.h"
#include BOARD_CONFIG

/* External functions */
extern void actuators_dronecan_init(struct dronecan_iface_t *iface);
extern void actuators_dronecan_commit(struct dronecan_iface_t *iface, int16_t *values, uint8_t nb);
extern void actuators_dronecan_cmd_commit(struct dronecan_iface_t *iface, int16_t *values, uint8_t nb);

#endif /* ACTUATORS_DRONECAN_H */
