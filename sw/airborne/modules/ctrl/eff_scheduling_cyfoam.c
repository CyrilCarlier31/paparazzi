/*
 * Copyright (C) 2017 Ewoud Smeur <ewoud_smeur@msn.com>
 *
 * This file is part of paparazzi.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/** @file modules/ctrl/ctrl_effectiveness_scheduling.c
 * Module that interpolates gainsets in flight based on the transition percentage
 */

#include "modules/ctrl/eff_scheduling_cyfoam.h"
#include "firmwares/rotorcraft/stabilization/stabilization_indi.h"
#include "firmwares/rotorcraft/guidance/guidance_h.h"
#include "generated/airframe.h"
#include "state.h"
#include "modules/radio_control/radio_control.h"

#if STABILIZATION_INDI_ALLOCATION_PSEUDO_INVERSE
#error "You need to use WLS control allocation for this module"
#endif

#ifndef INDI_FUNCTIONS_RC_CHANNEL
#error "You need to define an RC channel to switch between simple and advanced scheduling"
#endif

#ifdef FWD_G1
static float g1g2_forward[INDI_OUTPUTS][INDI_NUM_ACT] = FWD_G1;
#else
static float g1g2_forward[INDI_OUTPUTS][INDI_NUM_ACT] = {
  FWD_G1_ROLL,
  FWD_G1_PITCH,
  FWD_G1_YAW,
  FWD_G1_THRUST
};
#endif

#ifdef STABILIZATION_INDI_G1
static float g1g2_hover[INDI_OUTPUTS][INDI_NUM_ACT] = STABILIZATION_INDI_G1;
#else
static float g1g2_hover[INDI_OUTPUTS][INDI_NUM_ACT] = {
  STABILIZATION_INDI_G1_ROLL,
  STABILIZATION_INDI_G1_PITCH,
  STABILIZATION_INDI_G1_YAW,
  STABILIZATION_INDI_G1_THRUST
};
#endif

static float g2_both[INDI_NUM_ACT] = STABILIZATION_INDI_G2; //scaled by INDI_G_SCALING

void eff_scheduling_cyfoam_init(void)
{
  //sum of G1 and G2
  int8_t i;
  int8_t j;
  for (i = 0; i < INDI_OUTPUTS; i++) {
    for (j = 0; j < INDI_NUM_ACT; j++) {
      if (i != 2) {
        g1g2_hover[i][j] = g1g2_hover[i][j] / INDI_G_SCALING;
        g1g2_forward[i][j] = g1g2_forward[i][j] / INDI_G_SCALING;
      } else {
        g1g2_forward[i][j] = (g1g2_forward[i][j] + g2_both[j]) / INDI_G_SCALING;
        g1g2_hover[i][j] = (g1g2_hover[i][j] + g2_both[j]) / INDI_G_SCALING;
      }
    }
  }
}

static void eff_scheduling_periodic_a(void)
{
  int8_t i;
  int8_t j;
  const float ratio = stabilization.transition_ratio;
  for (i = 0; i < INDI_OUTPUTS; i++) {
    for (j = 0; j < INDI_NUM_ACT; j++) {
      g1g2[i][j] = g1g2_hover[i][j] * (1.0 - ratio) + g1g2_forward[i][j] * ratio;
    }
  }
}

static void eff_scheduling_periodic_b(void)
{
  float airspeed = stateGetAirspeed_f();
  struct FloatEulers eulers_zxy;
  if(airspeed < 6.0) {
    float_eulers_of_quat_zxy(&eulers_zxy, stateGetNedToBodyQuat_f());
    float pitch_interp = DegOfRad(eulers_zxy.theta);
    Bound(pitch_interp, -60.0, -30.0);
    float ratio = (pitch_interp + 30.0)/(-30.);

    /*pitch*/
    g1g2[1][0] = g1g2_hover[1][0]*(1-ratio) + -PITCH_EFF_AT_60/1000*ratio;
    g1g2[1][1] = g1g2_hover[1][1]*(1-ratio) +  PITCH_EFF_AT_60/1000*ratio;
    /*yaw*/
    g1g2[2][0] = g1g2_hover[2][0]*(1-ratio) + -YAW_EFF_AT_60/1000*ratio;
    g1g2[2][1] = g1g2_hover[2][1]*(1-ratio) + -YAW_EFF_AT_60/1000*ratio;
  } else {
    // calculate squared airspeed
    Bound(airspeed, 0.0, 30.0);
    float airspeed2 = airspeed*airspeed;

    float pitch_eff = CE_PITCH_A + CE_PITCH_B*airspeed2;
    g1g2[1][0] = -pitch_eff/1000;
    g1g2[1][1] =  pitch_eff/1000;

    float yaw_eff = CE_YAW_A + CE_YAW_B*airspeed2;
    g1g2[2][0] = -yaw_eff/1000;
    g1g2[2][1] = -yaw_eff/1000;
  }

  g1g2[0][2] = -actuator_state_filt_vect[2]/1000*SQUARED_ROLL_EFF;
  g1g2[0][3] =  actuator_state_filt_vect[3]/1000*SQUARED_ROLL_EFF;
  Bound(g1g2[0][2], -30.0/1000, -2.0/1000);
  Bound(g1g2[0][3], 2.0/1000,  30.0/1000);

  /*Make pitch gain equal to roll gain for turns forward flight*/
  if(airspeed > 12.0) {
    indi_gains.att.q = 107.0;
  } else {
    indi_gains.att.q = 200.0;
  }

}

void eff_scheduling_cyfoam_periodic(void)
{
  if (radio_control.values[INDI_FUNCTIONS_RC_CHANNEL] > 0) {
    eff_scheduling_periodic_b();
  } else {
    eff_scheduling_periodic_a();
  }

#ifdef INDI_THRUST_ON_PITCH_EFF
  //State prioritization {W Roll, W pitch, W yaw, TOTAL THRUST}
  if(radio_control.values[INDI_FUNCTIONS_RC_CHANNEL] > 0 && (actuator_state_filt_vect[0] < -7000) && (actuator_state_filt_vect[1] > 7000)) {
    Bwls[1][2] = INDI_THRUST_ON_PITCH_EFF/INDI_G_SCALING;
    Bwls[1][3] = INDI_THRUST_ON_PITCH_EFF/INDI_G_SCALING;
  } else if(radio_control.values[INDI_FUNCTIONS_RC_CHANNEL] > 0 && (actuator_state_filt_vect[0] > 7000) && (actuator_state_filt_vect[1] < -7000)) {
    Bwls[1][2] = -INDI_THRUST_ON_PITCH_EFF/INDI_G_SCALING;
    Bwls[1][3] = -INDI_THRUST_ON_PITCH_EFF/INDI_G_SCALING;
  } else {
    Bwls[1][2] = 0.0;
    Bwls[1][3] = 0.0;
  }
#endif
}

