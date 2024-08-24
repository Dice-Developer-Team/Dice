/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2021 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <cmath>

class DiceEst {
	//the face of dice
	int nFace{ 0 };
	int nSample{ 0 };
	int nSum{ 0 };
	int nSqrSum{ 0 };	//Sum of squares of samples
public:
	double expMean{ 0 };
	double estMean{ 0 };
	double expStd{ 0 };	//standard deviation
	double estStd{ 0 };
	double pZtest{ 0 };
	double pNormDist{ 0 };
	DiceEst(int face, int cnt, int sum, int sqr) :nFace(face), nSample(cnt), nSum(sum), nSqrSum(sqr) {
		if (face <= 1)return;
		//expect
		if (nFace == 100) {
			expMean = 50.5;
			expStd = 28.866;
		}
		expMean = ((double)nFace + 1) /2;
		for (int i = 1; i <= nFace; i++) {
			expStd += pow(i - expMean, 2);
		}
		expStd = sqrt(expStd / nFace);
		//estimate
		estMean = (double)nSum / nSample;
		if (nSample > 1) {
			estStd = sqrt((nSqrSum - expMean * expMean * nSample) / (double)nSample);
			double z_test{ (estMean - expMean) * sqrt(nSample) / expStd };
			pZtest = fabs(erf(z_test / sqrt(2)));
			pNormDist = 0.5 + 0.5 * erf(z_test / sqrt(2));
		}
	}
};