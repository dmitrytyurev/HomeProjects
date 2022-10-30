
#include "Level1Room.h"

void FillLevel1Room(std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	verts.resize(58);

	verts[0] = {464,840};
	verts[1] = { 468,840 };
	verts[2] = { 628,840 };
	verts[3] = { 634,840 };
	verts[4] = { 106,988 };
	verts[5] = { 140,1012 };
	verts[6] = { 190,1012 };
	verts[7] = { 464,988 };
	verts[8] = { 468,1012 };
	verts[9] = { 628,1012 };

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
		{{0,{0,0}, {""}, 0, 0, {""}, 0, 0},
		 {1,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		 {8,{0,0}, {""}, 0, 0, {""}, 0, 0},
		 {7,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[1] = { 1252,1120,1000,
		{{1,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{2,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{9,{0,1}, {"gray_wall"}, 0, 1.25, {""}, 0, 0},
		{8,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[1].edges[2].wallBrightsUp[0] = 0.4f;
	polies[1].edges[2].wallBrightsUp[1] = 0.4f;

	polies[2] = { 0,0,1252,
		{{2,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{3,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{10,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{9,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2}} };

	polies[3] = { 0,0,1252,
		{{4,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{7,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{8,{2,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{6,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[3].edges[2].wallBrightsDown[0] = 0.4f;
	polies[3].edges[2].wallBrightsDown[1] = 0.7f;

	polies[4] = { 0,0,1252,
		{{10,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{13,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{11,{2,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{9,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[4].edges[2].wallBrightsDown[0] = 0.7f;
	polies[4].edges[2].wallBrightsDown[1] = 0.4f;

	polies[5] = { 0,1250,1092,
		{{13,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{12,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1},
		{11,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[5].edges[1].wallBrightsDown[0] = 0.7f;
	polies[5].edges[1].wallBrightsDown[1] = 0.4f;

	polies[6] = { 0,1250,1092,
		{{12,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{13,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{31,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{30,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1}} };

	polies[7] = { 0,1250,1092,
		{{30,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{31,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{56,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{55,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1}} };

	polies[8] = { 0,1250,1092,
		{{56,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1},
		{55,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[8].edges[1].wallBrightsDown[0] = 0.4f;
	polies[8].edges[1].wallBrightsDown[1] = 0.7f;

	polies[9] = { 0,0,1252,
		{{53,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{56,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{52,{0,0}, {""}, 0, 0, {""}, 0, }} };
	polies[9].edges[0].wallBrightsDown[0] = 0.2f;
	polies[9].edges[0].wallBrightsDown[1] = 0.5f;

	polies[10] = { 0,0,1252,
		{{53,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{52,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{51,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[11] = { 0,0,1162,
		{{51,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{52,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{49,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{50,{2,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 1}} };

	polies[12] = { 0,0,1252,
		{{50,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{49,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{48,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[13] = { 0,0,1252,
		{{48,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{49,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{45,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{47,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2}} };
	polies[13].edges[3].wallBrightsDown[0] = 0.5f;
	polies[13].edges[3].wallBrightsDown[1] = 0.2f;

	polies[14] = { 0,1250,1092,
		{{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{45,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{46,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1}} };
	polies[14].edges[2].wallBrightsDown[0] = 0.7f;
	polies[14].edges[2].wallBrightsDown[1] = 0.4f;

	polies[15] = { 0,1250,1092,
		{{22,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{23,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1},
		{46,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{45,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[16] = { 0,1250,1092,
		{{4,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{5,{0,2}, {""}, 0, 0, {"panel2"}, 0, 1},
		{23,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{22,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[17] = { 0,1250,1092,
	{{4,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{6,{0,0.125f}, {""}, 0, 0, {"panel2"}, 0, 1},
		{5,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[17].edges[1].wallBrightsDown[0] = 0.4f;
	polies[17].edges[1].wallBrightsDown[1] = 0.7f;

	polies[18] = { 1252,1090,1035,
		{{5,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{6,{0,2}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1},
		{24,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{23,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"panel_light"}, {"panel"} };

	polies[19] = { 0,1250,1000,
		{{6,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{8,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{15,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{14,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[19].edges[1].brightFloor = 100;
	polies[19].edges[2].brightFloor = 130;

	polies[20] = { 0,1250,1000,
		{{8,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{9,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{16,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{20,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{19,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{15,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[20].edges[0].brightFloor = 100;
	polies[20].edges[1].brightFloor = 100;
	polies[20].edges[2].brightFloor = 130;
	polies[20].edges[3].brightFloor = 130;
	polies[20].edges[4].brightFloor = 130;
	polies[20].edges[5].brightFloor = 130;

	polies[21] = { 0,1250,1000,
		{{9,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{11,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{17,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{16,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[21].edges[0].brightFloor = 100;
	polies[21].edges[3].brightFloor = 130;

	polies[22] = { 0,1250,1000,
		{{11,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{21,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{17,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[23] = { 1252,1120,1000,
		{{16,{0.5f,0.25f}, {"monitors"}, 0, 1, {""}, 0, 0, 0, 0},
		{17,{0,1}, {"monitors"}, 0, 1, {""}, 0, 0, 1, 0},
		{21,{0.25f,0.5f}, {"monitors"}, 0, 1, {""}, 0, 0, 1 ,3},
		{20,{1,0}, {"monitors"}, 0, 1, {""}, 0, 0, 0, 3}}, {"ceil_lights"}, {"hexa_floor"} };
	polies[23].edges[0].brightFloor = 130;
	polies[23].edges[3].brightFloor = 130;

	polies[24] = { 0,1250,1000,
		{{21,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{28,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[25] = { 0,1250,1000,
		{{20,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{21,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{28,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[25].edges[0].brightFloor = 130;

	polies[26] = { 1252,1090,1035,
		{{11,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{12,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{30,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,2}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1}}, {"panel_light"}, {"panel"} };

	polies[27] = { 1252,1090,1035,
		{{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{30,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{55,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{1,0}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1}}, {"panel_light"}, {"panel"} };

	polies[28] = { 0,1250,1000,
		{{28,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{29,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{37,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[28].edges[2].brightFloor = 60;
	polies[28].edges[3].brightFloor = 150;

	polies[29] = { 0,1250,1000,
		{{37,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{36,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[29].edges[0].brightFloor = 150;
	polies[29].edges[1].brightFloor = 60;
	polies[29].edges[2].brightFloor = 60;
	polies[29].edges[3].brightFloor = 150;

	polies[30] = { 0,1250,1000,
		{{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{54,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{53,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[30].edges[0].brightFloor = 60;
	polies[30].edges[1].brightFloor = 60;
	polies[30].edges[2].brightFloor = 60;
	polies[30].edges[2].brightCeil = 60;

	polies[31] = { 0,0,1252,
		{{43,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{44,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{53,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{51,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0}} };
	polies[31].edges[1].wallBrightsDown[0] = 0.4f;
	polies[31].edges[1].wallBrightsDown[1] = 0.2f;

	polies[32] = { 0,0,1252,
		{{41,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{43,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0}} };

	polies[33] = { 0,0,1252,
		{{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{41,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{39,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[34] = { 0,0,1252,
		{{36,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{44,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{39,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{35,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[34].edges[0].wallBrightsDown[0] = 0.7f;
	polies[34].edges[0].wallBrightsDown[1] = 0.4f;

	polies[35] = { 0,0,1252,
		{{27,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{36,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{35,{1,0}, {""}, 0, 0, {"metal_column"}, 0, 2}} };
	polies[35].edges[2].wallBrightsDown[0] = 0.6f;
	polies[35].edges[2].wallBrightsDown[1] = 0.6f;

	polies[36] = { 0,0,1252,
		{{28,{0.234f,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{37,{0.063f,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{36,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0.234f}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2}} };
	polies[36].edges[3].wallBrightsDown[0] = 0.6f;
	polies[36].edges[3].wallBrightsDown[1] = 0.6f;

	polies[37] = { 1252,1160,1040,
		{{42,{2,0}, {""}, 0, 0, {"steps_light"}, 0, 1},
		{43,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{51,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{50,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };

	polies[38] = { 1252,1160,1030,
		{{40,{2,0}, {""}, 0, 0, {"steps_light"}, 0, 1},
		{41,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{43,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{42,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };

	polies[39] = { 1252,1160,1020,
		{{38,{2,0}, {""}, 0, 0, {"steps_light"}, 0, 1},
		{39,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{41,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{40,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };

	polies[40] = { 1252,1160,1010,
		{{34,{2,0}, {"cyber_wall_blue"}, 0, 1, {"steps_light"}, 0, 1},
		{35,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{39,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{38,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"ceil_lights"}, {"blue_hexa_floor"} };
	polies[40].edges[0].wallBrightsUp[0] = 0.6f;
	polies[40].edges[0].wallBrightsUp[1] = 0.6f;

	polies[41] = { 0,1250,1000,
		{{26,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{35,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{34,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[42] = { 0,1250,1000,
		{{19,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{20,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{27,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{26,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[42].edges[0].brightFloor = 130;
	polies[42].edges[1].brightFloor = 130;

	polies[43] = { 1252,1120,1000,
		{{14,{0.5f,0.75f}, {"monitors"}, 0, 1, {""}, 0, 0 , 0, 0},
		{15,{1,0}, {"monitors"}, 0, 1, {""}, 0, 0, 1, 0},
		{19,{0.25f,0.5f}, {"monitors"}, 0, 1, {""}, 0, 0, 1, 3},
		{18,{1,0}, {"monitors"}, 0, 1, {""}, 0, 0, 0, 3}}, {"ceil_lights"}, {"hexa_floor"} };
	polies[43].edges[1].brightFloor = 130;
	polies[43].edges[2].brightFloor = 130;

	polies[44] = { 0,1250,1000,
		{{18,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{19,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{26,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{25,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[44].edges[1].brightFloor = 130;

	polies[45] = { 0,0,1252,
		{{25,{0.234f,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{26,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{33,{0,0.063f}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2},
		{32,{0,0.234f}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 2}} };
	polies[45].edges[0].wallBrightsDown[0] = 0.6f;
	polies[45].edges[0].wallBrightsDown[1] = 0.6f;

	polies[46] = { 0,0,1252,
		{{26,{0,1}, {""}, 0, 0, {"metal_column"}, 0, 2},
		{34,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{33,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[46].edges[0].wallBrightsDown[0] = 0.6f;
	polies[46].edges[0].wallBrightsDown[1] = 0.6f;

	polies[47] = { 0,0,1252,
		{{34,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{38,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2},
		{33,{0,0}, {""}, 0, 0, {""}, 0, 0}} };
	polies[47].edges[2].wallBrightsDown[0] = 0.4f;
	polies[47].edges[2].wallBrightsDown[1] = 0.7f;

	polies[48] = { 0,0,1252,
		{{57,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{38,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{40,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[49] = { 0,0,1252,
		{{40,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{42,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

	polies[50] = { 0,0,1252,
		{{57,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{42,{0,0}, {""}, 0, 0, {"cyber_wall_blue"}, 0, 0},
		{50,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{48,{1,0}, {""}, 0, 0, {"gray_wall"}, 0, 2}} };
	polies[50].edges[3].wallBrightsDown[0] = 0.2f;
	polies[50].edges[3].wallBrightsDown[1] = 0.4f;

	polies[51] = { 0,1250,1000,
		{{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{48,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[51].edges[0].brightFloor = 60;
	polies[51].edges[1].brightFloor = 60;
	polies[51].edges[2].brightFloor = 60;
	polies[51].edges[2].brightCeil = 60;

	polies[52] = { 0,1250,1000,
		{{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{32,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{33,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{57,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[52].edges[0].brightFloor = 60;
	polies[52].edges[1].brightFloor = 150;
	polies[52].edges[2].brightFloor = 150;
	polies[52].edges[3].brightFloor = 60;

	polies[53] = { 0,1250,1000,
		{{24,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{25,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{32,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{47,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };
	polies[53].edges[2].brightFloor = 150;
	polies[53].edges[3].brightFloor = 60;

	polies[54] = { 0,1250,1000,
		{{18,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{25,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{24,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[55] = { 0,1250,1000,
		{{6,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{14,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{18,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{24,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"stone_wall"}, {"hexa_floor"} };

	polies[56] = { 1252,1090,1035,
		{{23,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{24,{1,0}, {"big_wall"}, 0, 1, {"metal_plates2"}, 0, 1},
		{47,{0,0}, {""}, 0, 0, {""}, 0, 0},
		{46,{0,0}, {""}, 0, 0, {""}, 0, 0}}, {"panel_light"}, {"panel"} };

}
