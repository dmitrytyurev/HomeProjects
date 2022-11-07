#include "Level3Arena.h"

void FillLevel3Arena(std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	verts.resize(195);

	verts[84] = { 1150, 630};
	verts[85] = { 1248, 658};
	verts[86] = { 1270, 738};
	verts[87] = { 1248, 818};
	verts[88] = { 1050, 895};
	verts[89] = { 1270, 895 };

	verts[90] = { 1074, 977};
	verts[91] = { 1248, 977 };
	verts[92] = { 1050, 1064};
	verts[93] = { 1248, 1136};
	verts[94] = { 1298, 1136 };
	verts[95] = { 1074, 1136 };
	verts[96] = { 1050, 1220};
	verts[97] = { 1074, 1295};
	verts[98] = { 1248, 1295 };
	verts[99] = { 1298, 1295 };

	verts[100] = { 1248, 1320};
	verts[101] = { 1298, 7};
	verts[102] = { 1646, 7};
	verts[103] = { 1298, 339};
	verts[104] = { 1518, 372};
	verts[105] = { 1298, 598};
	verts[106] = { 1438, 598};
	verts[107] = { 2228, 300};
	verts[108] = { 1989, 595};
	verts[109] = { 1835, 785};

	verts[110] = { 2795, 737};
	verts[111] = { 2529, 881};
	verts[112] = { 2319, 996};
	verts[113] = { 2067, 1059};
	verts[114] = { 1972, 996};
	verts[115] = { 1972, 1422};
	verts[116] = { 2067, 1360 };
	verts[117] = { 2319, 1422 };
	verts[118] = { 2529, 1537};
	verts[119] = { 2795, 1681};

	verts[120] = { 2228, 2118};
	verts[121] = { 1989, 1823};
	verts[122] = { 1835, 1633};
	verts[123] = { 1438, 1820};
	verts[124] = { 1518, 2046};
	verts[125] = { 1646, 2411};
	verts[126] = { 1298, 2411};
	verts[127] = { 1298, 2079};
	verts[128] = { 1298, 1820};

	verts[129] = { 1298, 25};
	verts[130] = { 1641, 25};
	verts[131] = { 2213, 319};
	verts[132] = { 2770, 751};
	verts[133] = { 2770, 1668};
	verts[134] = { 2213, 2099};
	verts[135] = { 1641, 2393};
	verts[136] = { 1298, 2393};
	verts[137] = { 1610, 232};
	verts[138] = { 1756, 296};
	verts[139] = { 1921, 382};

	verts[140] = { 2039, 459};
	verts[141] = { 2009, 507};
	verts[142] = { 1893, 435};
	verts[143] = { 1731, 349};
	verts[144] = { 1582, 288};
	verts[145] = { 1723, 607};
	verts[146] = { 1763, 607};
	verts[147] = { 1763, 647};
	verts[148] = { 1723, 647};
	verts[149] = { 1991, 830};

	verts[150] = { 1991, 871};
	verts[151] = { 2031, 830};
	verts[152] = { 2031, 871};
	verts[153] = { 2156, 531};
	verts[154] = { 2110, 571};
	verts[155] = { 2290, 690};
	verts[156] = { 2242, 729};
	verts[157] = { 2357, 766};
	verts[158] = { 2317, 801};
	verts[159] = { 2432, 858};

	verts[160] = { 2389, 890};
	verts[161] = { 2107, 1058};
	verts[162] = { 2107, 1099};
	verts[163] = { 2067, 1099};
	verts[164] = { 2107, 1319};
	verts[165] = { 2067, 1319};
	verts[166] = { 2107, 1360};
	verts[167] = { 2576, 1079};
	verts[168] = { 2629, 1079 };
	verts[169] = { 2629, 1339};

	verts[170] = { 2576, 1339 };
	verts[171] = { 1991, 1547};
	verts[172] = { 2031, 1547 };
	verts[173] = { 2031, 1587};
	verts[174] = { 1991, 1587 };
	verts[175] = { 1723, 1771};
	verts[176] = { 1763, 1771 };
	verts[177] = { 1763, 1811};
	verts[178] = { 1723, 1811 };
	verts[179] = { 2389, 1528};

	verts[180] = { 2431, 1560};
	verts[181] = { 2357, 1652};
	verts[182] = { 2317, 1617};
	verts[183] = { 2289, 1728};
	verts[184] = { 2241, 1689};
	verts[185] = { 2156, 1887};
	verts[186] = { 2110, 1846};
	verts[187] = { 2040, 1959};
	verts[188] = { 2010, 1911};
	verts[189] = { 1920, 2036};

	verts[190] = { 1893, 1983};
	verts[191] = { 1755, 2123};
	verts[192] = { 1730, 2069};
	verts[193] = { 1609, 2186};
	verts[194] = { 1581, 2130};



	polies.resize(121);

	polies[84] = { 0,0,1122,
	{{64,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {84,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {85,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[85] = { 0,1120,1000,
	{{64,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {85,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {87,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {63,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[86] = { 0,0,1122,
	{{85,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {86,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {87,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[87] = { 0,1120,1000,
	{{63,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {87,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {91,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {90,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[88] = { 0,0,1122,
	{{87,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {89,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {91,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[89] = { 0,1120,1000,
	{{90,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {91,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {93,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {95,{0,0}, {""}, 0, 0, {""}, 0, 0}} };




	polies[90] = { 0,0,1122,
	{{91,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {94,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {93,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[91] = { 0,1120,1000,
	{{95,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {93,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {98,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {97,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[92] = { 0,1120,1000,
	{{93,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {94,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {99,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {98,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[93] = { 0,0,1122,
	{{97,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {98,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {100,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[94] = { 0,0,1122,
	{{98,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {99,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {100,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[95] = { 0,0,1122,
	{{63,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {90,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {88,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[96] = { 0,0,1122,
	{{90,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {95,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {92,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[97] = { 0,0,1122,
	{{95,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {97,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {96,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[98] = { 0,2000,1100,
	{{104,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {108,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {109,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {106,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[99] = { 1600,1550,1250,
	{{107,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {110,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {111,{0,0}, {""}, 0, 0, {""}, 0, 0}} };



	polies[100] = { 1600,1550,1250,                  //
	{{107,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {111,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {112,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {108,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[101] = { 1600,1550,1100,
	{{108,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {112,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {113,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[102] = { 1600,1550,1100,                // 
	{{109,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {108,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {113,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {114,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[103] = { 0,2000,1000,
	{{113,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {116,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {115,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {114,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[104] = { 1600,1550,1153,                      //
	{{112,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {117,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {116,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {113,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[105] = { 1600,1550,1000,
	{{111,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {118,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {117,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {112,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[106] = { 1600,1550,1400,                     //
	{{110,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {119,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {118,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {111,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[107] = { 1600,1550,1250,
	{{119,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {120,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {118,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[108] = { 1600,1550,1250,
	{{118,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {120,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {121,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {117,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[109] = { 1600,1550,1100,
	{{117,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {121,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {116,{0,0}, {""}, 0, 0, {""}, 0, 0}} };


	polies[110] = { 1600,1550,1100,
	{{116,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {121,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {122,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {115,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[111] = { 1600,1550,1250,
	{{120,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {125,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {124,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {121,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[112] = { 0,2000,1100,
	{{121,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {124,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {123,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {122,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[113] = { 1600,1550,1250,
	{{125,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {126,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {127,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {124,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[114] = { 0,2000,1100,
	{{124,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {127,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {128,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {123,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[115] = { 0,2000,1000,
	{{99,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {115,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {122,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {123,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {128,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[116] = { 0,2000,1000,
	{{94,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {114,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {115,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {99,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[117] = { 0,2000,1000,
	{{105,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {106,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {109,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {114,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {94,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[118] = { 1600,1550,1250,
	{{101,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {102,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {104,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {103,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[119] = { 1600,1550,1250,
	{{102,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {107,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {108,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {104,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[120] = { 0,2000,1100,
	{{103,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {104,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {106,{0,0}, {""}, 0, 0, {""}, 0, 0},
	 {105,{0,0}, {""}, 0, 0, {""}, 0, 0}} };


}
