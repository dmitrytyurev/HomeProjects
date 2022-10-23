
#include "LevelData.h"

void FillLevelData(std::vector<FPoint2D>& verts, std::vector<Poly>& polies)
{
	verts.resize(58);

	verts[0] = {464,840};
	verts[1] = { 488,840 };
	verts[2] = { 608,840 };
	verts[3] = { 634,840 };
	verts[4] = { 106,988 };
	verts[5] = { 140,1012 };
	verts[6] = { 190,1012 };
	verts[7] = { 464,988 };
	verts[8] = { 488,1012 };
	verts[9] = { 608,1012 };

	verts[10] = { 634,988 };
	verts[11] = { 910,1012 };
	verts[12] = { 960,1012 };
	verts[13] = { 990,988 };
	verts[14] = { 310,1192 };
	verts[15] = { 390,1192 };
	verts[16] = { 718,1192 };
	verts[17] = { 796,1192 };
	verts[18] = { 310,1398 };
	verts[19] = { 390,1398 };

	verts[20] = { 718,1398 };
	verts[21] = { 796,1398 };
	verts[22] = { 106,1586 };
	verts[23] = { 140,1586 };
	verts[24] = { 190,1586 };
	verts[25] = { 342,1586 };
	verts[26] = { 388,1586 };
	verts[27] = { 718,1586 };
	verts[28] = { 764,1586 };
	verts[29] = { 910,1586 };

	verts[30] = { 960,1586 };
	verts[31] = { 990,1586 };
	verts[32] = { 342,1632 };
	verts[33] = { 364,1632 };
	verts[34] = { 388,1632 };
	verts[35] = { 718,1632 };
	verts[36] = { 742,1632 };
	verts[37] = { 764,1632 };
	verts[38] = { 388,1670 };
	verts[39] = { 718,1670 };

	verts[40] = { 388,1709 };
	verts[41] = { 718,1709 };
	verts[42] = { 388,1746 };
	verts[43] = { 718,1746 };
	verts[44] = { 742,1746 };
	verts[45] = { 106,1943 };
	verts[46] = { 140,1916 };
	verts[47] = { 190,1916 };
	verts[48] = { 364,1916 };
	verts[49] = { 370,1943 };

	verts[50] = { 388,1916 };
	verts[51] = { 718,1916 };
	verts[52] = { 730,1943 };
	verts[53] = { 742,1916 };
	verts[54] = { 910,1916 };
	verts[55] = { 960,1916 };
	verts[56] = { 990,1943 };
	verts[57] = { 364,1746 };

	polies.resize(57);

	polies[0] = { 0,0,1252,
		{{0,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		 {1,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		 {8,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		 {7,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[1] = { 0,1250,1000,
		{{1,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{2,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{9,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{8,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[2] = { 0,0,1252,
		{{2,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{3,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{10,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{9,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[3] = { 0,0,1252,
		{{4,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{7,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{8,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{6,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[4] = { 0,0,1252,
		{{10,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{13,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{11,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{9,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[5] = { 0,1250,1035,
		{{13,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{12,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{11,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[6] = { 0,1250,1092,
		{{12,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{13,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{31,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{30,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[7] = { 0,1250,1092,
		{{30,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{31,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{56,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{55,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[8] = { 0,1250,1092,
		{{56,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{54,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{55,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[9] = { 0,0,1252,
		{{53,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{54,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{56,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{52,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[10] = { 0,0,1252,
		{{53,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{52,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{51,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[11] = { 0,0,1252,
		{{51,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{52,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{49,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{50,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[12] = { 0,0,1252,
		{{50,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{49,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{48,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[13] = { 0,0,1252,
		{{48,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{49,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{45,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{43,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[14] = { 0,1250,1092,
		{{47,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{45,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{46,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[15] = { 0,1250,1092,
		{{22,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{23,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{46,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{45,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[16] = { 0,1250,1092,
		{{4,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{5,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{23,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{22,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[17] = { 0,1250,1035,
		{{4,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{6,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{5,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[18] = { 1252,1090,1035,
		{{5,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{6,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{24,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{23,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[19] = { 0,1250,1000,
		{{6,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{8,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{15,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{14,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[20] = { 0,1250,1000,
		{{8,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{9,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{16,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{20,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{19,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{15,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[21] = { 0,1250,1000,
		{{9,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{11,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{17,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{16,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[22] = { 0,1250,1000,
		{{11,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{29,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{21,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{17,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[23] = { 1252,1120,1000,
		{{16,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{17,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{21,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{20,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[24] = { 0,1250,1000,
		{{21,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{29,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{28,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[25] = { 0,1250,1000,
		{{20,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{21,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{28,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{27,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[26] = { 1252,1090,1035,
		{{11,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{12,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{30,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{29,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[27] = { 1252,1090,1035,
		{{29,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{30,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{55,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{54,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[28] = { 0,1250,1000,
		{{28,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{29,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{54,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{37,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[29] = { 0,1250,1000,
		{{37,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{54,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{44,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{36,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[30] = { 0,1250,1000,
		{{44,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{54,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{53,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[31] = { 0,0,1252,
		{{43,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{44,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{53,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{51,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[32] = { 0,0,1252,
		{{41,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{44,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{43,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[33] = { 0,0,1252,
		{{44,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{41,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{39,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[34] = { 0,0,1252,
		{{36,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{44,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{39,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{35,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[35] = { 0,0,1252,
		{{27,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{36,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{35,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[36] = { 0,0,1252,
		{{28,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{37,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{36,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{27,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[37] = { 1252,1160,1040,
		{{42,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{43,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{51,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{50,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[38] = { 1252,1160,1030,
		{{40,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{41,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{43,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{42,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[39] = { 1252,1160,1020,
		{{38,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{39,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{41,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{40,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[40] = { 1252,1160,1010,
		{{34,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{35,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{39,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{38,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[41] = { 0,1250,1000,
		{{26,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{27,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{35,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{34,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[42] = { 0,1250,1000,
		{{19,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{20,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{27,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{26,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[43] = { 1252,1120,1000,
		{{14,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{15,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{19,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{18,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[44] = { 0,1250,1000,
		{{18,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{19,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{26,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{25,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[45] = { 0,0,1252,
		{{25,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{26,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{33,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{32,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[46] = { 0,0,1252,
		{{26,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{34,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{33,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[47] = { 0,0,1252,
		{{34,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{38,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{57,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{33,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[48] = { 0,0,1252,
		{{57,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{38,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{40,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[49] = { 0,0,1252,
		{{40,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{42,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{57,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[50] = { 0,0,1252,
		{{57,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{42,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{50,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{48,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[51] = { 0,1250,1000,
		{{47,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{57,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{48,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[52] = { 0,1250,1000,
		{{47,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{32,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{33,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{57,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[53] = { 0,1250,1000,
		{{24,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{25,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{32,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{47,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[54] = { 0,1250,1000,
		{{18,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{25,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{24,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[55] = { 0,1250,1000,
		{{6,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{14,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{18,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{24,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	polies[56] = { 1252,1090,1035,
		{{23,0,0,{0,0}, 0, 0, 0, 0,    0 ,0 },
		{24,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{47,0,0,{0,0}, 0, 0, 0, 0,    0, 0},
		{46,0,0,{0,0}, 0, 0, 0, 0,    0, 0}} };

	// Заполнение  poly.yCeil и poly.yRoof

	for (int i = 0; i < polies.size(); ++i) {
		Poly& poly = polies[i];
		if (poly.yCeil < poly.yFloor)
			poly.yCeil = poly.yFloor + 10;
		if (poly.yRoof < poly.yCeil)
			poly.yRoof = poly.yCeil + 10;
	}

	// Заполнение инфы о смежности
	for (int pn1 = 0; pn1 < polies.size(); ++pn1) {
		Poly& poly1 = polies[pn1];
		int vertsNum1 = (int)poly1.edges.size();
		for (int en1 = 0; en1 < vertsNum1; ++en1) {
			poly1.edges[en1].adjPolyN = -1;
		}
	}

	for (int pn1 = 0; pn1 < polies.size(); ++pn1) {
		Poly& poly1 = polies[pn1];
		int vertsNum1 = (int)poly1.edges.size();
		for (int en1 = 0; en1 < vertsNum1; ++en1) {
			int vertInd1 = poly1.edges[en1].firstInd;
			int vertInd2 = poly1.edges[(en1+1)% vertsNum1].firstInd;
			for (int pn2 = pn1+1; pn2 < polies.size(); ++pn2) {
				Poly& poly2 = polies[pn2];
				int vertsNum2 = (int)poly2.edges.size();
				for (int en2 = 0; en2 < vertsNum2; ++en2) {
					if (poly2.edges[en2].firstInd == vertInd2 && poly2.edges[(en2 + 1) % vertsNum2].firstInd == vertInd1) {
						poly1.edges[en1].adjPolyN = pn2;
						poly1.edges[en1].adjEdgeN = en2;
						poly2.edges[en2].adjPolyN = pn1;
						poly2.edges[en2].adjEdgeN = en1;
						goto m1;
					}
				}
			}
m1:;
		}
	}

	// Тестовое заполнение текстурных координат

	for (int pn1 = 0; pn1 < polies.size(); ++pn1) {
		Poly& poly1 = polies[pn1];
		int vertsNum1 = (int)poly1.edges.size();
		for (int en1 = 0; en1 < vertsNum1; ++en1) {
			poly1.edges[en1].u[0] = 0;

			int nextInd = (en1 + 1) % vertsNum1;
			double dx = verts[poly1.edges[en1].firstInd].x - verts[poly1.edges[nextInd].firstInd].x;
			double dz = verts[poly1.edges[en1].firstInd].z - verts[poly1.edges[nextInd].firstInd].z;
			double dist = sqrt(dx * dx + dz * dz);
			poly1.edges[en1].u[1] = float(dist * 0.01);

			poly1.edges[en1].vCeil = 1;
			poly1.edges[en1].vCeilAdd = 0.01f;
			poly1.edges[en1].vFloor = 0;
			poly1.edges[en1].vFloorAdd = 0.01f;

			poly1.edges[en1].uFloorCeil = float(verts[poly1.edges[en1].firstInd].x * 0.01);
			poly1.edges[en1].vFloorCeil = float(verts[poly1.edges[en1].firstInd].z * 0.01);
		}
	}

	// Разворачиваем систему координат, чтобы был обход по часовой стрелке
	for (FPoint2D& vert: verts) {
		vert.z = -vert.z;
	}
}
