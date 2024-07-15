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

#ifndef ACTUATORS_DRONECAN1_H
#define ACTUATORS_DRONECAN1_H

#include "actuators_dronecan.h"

/** Stub file needed per dronecan interface because of generator */
extern int16_t actuators_dronecan1_values[SERVOS_DRONECAN1_NB];

#if USE_NPS
#define ActuatorsDronecan1Init() {}
#define ActuatorDronecan1Set(_i, _v) {}
#define ActuatorsDronecan1Commit()  {}
#else
#define ActuatorsDronecan1Init() actuators_dronecan_init(&dronecan1)
#define ActuatorDronecan1Set(_i, _v) { actuators_dronecan1_values[_i] = _v; }
#define ActuatorsDronecan1Commit()  actuators_dronecan_commit(&dronecan1, actuators_dronecan1_values, SERVOS_DRONECAN1_NB)
#endif

#endif /* ACTUATORS_DRONECAN1_H */