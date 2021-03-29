/* Copyright 2018-2020 Nick Brassel (@tzarc)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <quantum.h>
#include <qp.h>

//----------------------------------------------------------
// Layout

// clang-format off

#define LAYOUT_all( \
        k00, k01, k02, k03, k04, k05,                                   k06, k07, k08, k09, k0A, k0B, \
        k10, k11, k12, k13, k14, k15,                                   k16, k17, k18, k19, k1A, k1B, \
        k20, k21, k22, k23, k24, k25,                                   k26, k27, k28, k29, k2A, k2B, \
        k30, k31, k32, k33, k34, k35,                                   k36, k37, k38, k39, k3A, k3B, \
                       k40, k41, k42, k43,                         k44, k45, k46, k47,                  \
                                    k50, k51,                   k52, k53,                    \
                              k60,                                      k61,                          \
                         k70, k71, k72,                            k73, k74, k75,                     \
                              k80,                                      k81                           \
    )                                                       \
    {                                                       \
        {	k05	,	k04	,	k03	,	k02	,	k01	,	k00	}, \
        {	k15	,	k14	,	k13	,	k12	,	k11	,	k10	}, \
        {	k25	,	k24	,	k23	,	k22	,	k21	,	k20	}, \
        {	k35	,	k34	,	k33	,	k32	,	k31	,	k30	}, \
        {	k43	,	k51	,	k50	,	k42	,	k41	,	k40	}, \
        {	k60 ,   k80 ,   k72 ,   k70 ,   k71 ,   KC_NO	}, \
        {	k06	,	k07	,	k08	,	k09	,	k0A	,	k0B	}, \
        {	k16	,	k17	,	k18	,	k19	,	k1A	,	k1B	}, \
        {	k26	,	k27	,	k28	,	k29	,	k2A	,	k2B	}, \
        {	k36	,	k37	,	k38	,	k39	,	k3A	,	k3B	}, \
        {	k44	,	k52	,	k53	,	k45	,	k46	,	k47	}, \
        {	KC_NO,	k74	,	k73 ,	k75	,	k81	,	k61	}, \
    }

// clang-format on



