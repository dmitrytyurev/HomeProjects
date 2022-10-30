#include "Level2Corridor.h"

void FillLevel2Corridor(std::vector<FPoint2D>& verts, std::vector<Poly>& polies, std::vector<Texture>& textures)
{
	verts.resize(84);

	verts[58] = { 628, 818 };
	verts[59] = { 693, 818};

	verts[60] = { 852, 818 };
	verts[61] = { 901, 818 };
	verts[62] = { 852, 840};
	verts[63] = { 1074, 818 };
	verts[64] = { 1074, 658 };
	verts[65] = { 901, 658 };
	verts[66] = { 852, 658 };
	verts[67] = { 693, 658};
	verts[68] = { 628, 658 };
	verts[69] = { 468, 658 };

	verts[70] = { 453, 658 };
	verts[71] = { 453, 818 };
	verts[72] = { 468, 818 };
	verts[73] = { 693, 609};
	verts[74] = { 852, 609 };
	verts[75] = { 693, 451};
	verts[76] = { 852, 451 };
	verts[77] = { 870, 451 };
	verts[78] = { 852, 359 };
	verts[79] = { 750, 340};

	verts[80] = { 292 , 451 };
	verts[81] = { 270, 550};
	verts[82] = { 292, 609 };
	verts[83] = { 693, 359};

	polies.resize(84);

	//polies[0] = { 0,0,1000,
	//	{{0,{0,0}, {""}, 0, 0, {""}, 0, 0},
	//	 {1,{1,0}, {""}, 0, 0, {""}, 0, 0},
	//	 {8,{0,0}, {""}, 0, 0, {""}, 0, 0},
	//	 {7,{0,0}, {""}, 0, 0, {""}, 0, 0}} };

}