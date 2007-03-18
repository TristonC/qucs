/*
 * tline4p.cpp - ideal 4-terminal transmission line class implementation
 *
 * Copyright (C) 2007 Stefan Jahn <stefan@lkcc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.  
 *
 * $Id: tline4p.cpp,v 1.3 2007/02/21 14:57:18 ela Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "complex.h"
#include "object.h"
#include "node.h"
#include "circuit.h"
#include "component_id.h"
#include "constants.h"
#include "matrix.h"
#include "tline4p.h"

tline4p::tline4p () : circuit (4) {
  type = CIR_TLINE4P;
}

void tline4p::calcSP (nr_double_t frequency) {
  nr_double_t l = getPropertyDouble ("L");
  nr_double_t z = getPropertyDouble ("Z");
  nr_double_t a = getPropertyDouble ("Alpha");
  nr_double_t b = 2 * M_PI * frequency / C0;
  a = log (a) / 2;

  complex g = rect (a, b);
  nr_double_t p = 2 * z0 + z;
  nr_double_t n = 2 * z0 - z;
  complex e = exp (2 * g * l);
  complex d = p * p * e - n * n;

  complex s11 = z * (p * e + n) / d;
  complex s14 = 1.0 - s11;
  complex s12 = 4.0 * z * z0 * exp (g * l) / d;

  setS (NODE_1, NODE_1, +s11); setS (NODE_2, NODE_2, +s11);
  setS (NODE_3, NODE_3, +s11); setS (NODE_4, NODE_4, +s11);
  setS (NODE_1, NODE_4, +s14); setS (NODE_4, NODE_1, +s14);
  setS (NODE_2, NODE_3, +s14); setS (NODE_3, NODE_2, +s14);
  setS (NODE_1, NODE_2, +s12); setS (NODE_2, NODE_1, +s12);
  setS (NODE_3, NODE_4, +s12); setS (NODE_4, NODE_3, +s12);
  setS (NODE_1, NODE_3, -s12); setS (NODE_3, NODE_1, -s12);
  setS (NODE_2, NODE_4, -s12); setS (NODE_4, NODE_2, -s12);
}

void tline4p::calcNoiseSP (nr_double_t) {
  nr_double_t l = getPropertyDouble ("L");
  if (l < 0) return;
  // calculate noise using Bosma's theorem
  nr_double_t T = getPropertyDouble ("Temp");
  matrix s = getMatrixS ();
  matrix e = eye (getSize ());
  setMatrixN (kelvin (T) / T0 * (e - s * transpose (conj (s))));
}

void tline4p::calcNoiseAC (nr_double_t) {
  nr_double_t l = getPropertyDouble ("L");
  if (l < 0) return;
  // calculate noise using Bosma's theorem
  nr_double_t T = getPropertyDouble ("Temp");
  setMatrixN (4 * kelvin (T) / T0 * real (getMatrixY ()));
}

void tline4p::initDC (void) {
  setVoltageSources (2);
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
  voltageSource (VSRC_2, NODE_3, NODE_4);
}

void tline4p::initAC (void) {
  nr_double_t l = getPropertyDouble ("L");
  if (l != 0.0) {
    setVoltageSources (0);
    allocMatrixMNA ();
  } else {
    setVoltageSources (2);
    allocMatrixMNA ();
    voltageSource (VSRC_1, NODE_1, NODE_2);
    voltageSource (VSRC_2, NODE_3, NODE_4);
  }
}

void tline4p::calcAC (nr_double_t frequency) {
  nr_double_t l = getPropertyDouble ("L");
  nr_double_t z = getPropertyDouble ("Z");
  nr_double_t a = getPropertyDouble ("Alpha");
  nr_double_t b = 2 * M_PI * frequency / C0;
  a = log (a) / 2;
  if (l != 0.0) {
    complex g = rect (a, b);
    complex y11 = coth (g * l) / z;
    complex y21 = -cosech (g * l) / z;
    setY (NODE_1, NODE_1, +y11); setY (NODE_2, NODE_2, +y11);
    setY (NODE_3, NODE_3, +y11); setY (NODE_4, NODE_4, +y11);
    setY (NODE_1, NODE_4, -y11); setY (NODE_4, NODE_1, -y11);
    setY (NODE_2, NODE_3, -y11); setY (NODE_3, NODE_2, -y11);
    setY (NODE_1, NODE_2, +y21); setY (NODE_2, NODE_1, +y21);
    setY (NODE_3, NODE_4, +y21); setY (NODE_4, NODE_3, +y21);
    setY (NODE_1, NODE_3, -y21); setY (NODE_3, NODE_1, -y21);
    setY (NODE_2, NODE_4, -y21); setY (NODE_4, NODE_2, -y21);
  }
}

void tline4p::initTR (void) {
  nr_double_t l = getPropertyDouble ("L");
  nr_double_t z = getPropertyDouble ("Z");
  deleteHistory ();
  if (l > 0.0) {
    setVoltageSources (2);
    allocMatrixMNA ();
    setHistory (true);
    initHistory (l / C0);
    setB (NODE_1, VSRC_1, +1); setB (NODE_2, VSRC_2, +1);
    setB (NODE_4, VSRC_1, -1); setB (NODE_3, VSRC_2, -1);
    setC (VSRC_1, NODE_1, +1); setC (VSRC_2, NODE_2, +1);
    setC (VSRC_1, NODE_4, -1); setC (VSRC_2, NODE_3, -1);
    setD (VSRC_1, VSRC_1, -z); setD (VSRC_2, VSRC_2, -z); 
  } else {
    setVoltageSources (2);
    allocMatrixMNA ();
    voltageSource (VSRC_1, NODE_1, NODE_2);
    voltageSource (VSRC_2, NODE_3, NODE_4);
  }
}

void tline4p::calcTR (nr_double_t t) {
  nr_double_t l = getPropertyDouble ("L");
  nr_double_t a = getPropertyDouble ("Alpha");
  nr_double_t z = getPropertyDouble ("Z");
  nr_double_t T = l / C0;
  a = log (a) / 2;
  if (T > 0.0) {
    T = t - T;
    a = exp (-a / 2 * l);
    setE (VSRC_1, a * (getV (NODE_2, T) - getV (NODE_3, T) +
		       z * getJ (VSRC_2, T)));
    setE (VSRC_2, a * (getV (NODE_1, T) - getV (NODE_4, T) +
		       z * getJ (VSRC_1, T)));
  }
}