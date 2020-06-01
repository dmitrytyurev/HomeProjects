

//--------------------------------------------------------------------------------------------
//FILE* f = fopen("WaveTable.txt", "wt");
//fprintf(f, "%f\n", waveTable[i]);
//fclose(f);

const int Scene2DSize = 200;
const int WaveTableSize = 1000;
float randomSpeedRotateAmpl;  // Амплитуда рандомного вращения скоростей
float randomSpeedRotateSpeed;  // Скорость рандомного вращения скоростей
float randomSpeedAccelerateAml; // Амплитуда андомного изменения скоростей

//--------------------------------------------------------------------------------------------

float waveTable[WaveTableSize];
float Scale;
double SpeedSlowdown;


//--------------------------------------------------------------------------------------------

struct Cell2D
{
	float smokeDens = 0;
	float airDens = 0;
	float speedX = 0;
	float speedY = 0;
	float avgSpeedX = 0;
	float avgSpeedY = 0;

	float smokeDensAdd = 0;
	float airDensAdd = 0;
	float speedXAdd = 0;
	float speedYAdd = 0;
	int waveTableRandomOffset = 0;
	int avgWaveTableRandomOffset = 0;
};

//--------------------------------------------------------------------------------------------

Cell2D scene2D[Scene2DSize][Scene2DSize];

//--------------------------------------------------------------------------------------------

void turnPoint(float x, float y, double angle, float& newX, float& newY)
{
	double s = sin(angle);
	double c = cos(angle);
	newX = (float)(x * c - y * s);
	newY = (float)(x * s + y * c);
}


//--------------------------------------------------------------------------------------------

void initWaveTable()
{
	for (int i = 0; i < WaveTableSize; ++i) {
		float angle = ((float)i) / WaveTableSize * 2 * PI;
		waveTable[i] = sin(angle) + sin(angle * 2) + sin(angle * 3) + sin(angle * 4);
	}
}

//--------------------------------------------------------------------------------------------

void save2DSceneToBmp(const std::string& fileName)
{
	uint8_t* bmpData = new uint8_t[Scene2DSize * Scene2DSize * 3];
	for (int y = 0; y < Scene2DSize; ++y)
	{
		for (int x = 0; x < Scene2DSize; ++x)
		{
			uint8_t bright = std::min(int(scene2D[x][y].smokeDens * 255), 255);          // Плотности 2d-сцены
			//uint8_t bright = std::min(fabs(scene2D[x][y].speedX * 128), 255.f);          // Карта скоростей 2d-сцены
			int pixelOffs = (y * Scene2DSize + x) * 3;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
			bmpData[pixelOffs++] = bright;
		}
	}

	save_bmp24(fileName.c_str(), Scene2DSize, Scene2DSize, (const char *)bmpData);
	delete[] bmpData;

}
//--------------------------------------------------------------------------------------------

void update2dScene(int stepN)
{
	std::vector<float> coeffs; // Коэффициенты зацепления зааффекченных клатов для нормализации

	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			Cell2D& cell = scene2D[x][y];
			// Расчитаем новые координаты углов текущей клетки после сдвига и расширения
			float x1 = (float)x;
			float y1 = (float)y;
			float x2 = x1 + 1.f;
			float y2 = y1 + 1.f;

			//float xCenter = x1 + 0.5f;
			//float yCenter = y1 + 0.5f;
			//x1 = (x1 - xCenter) * Scale + xCenter + cell.speedX;
			//y1 = (y1 - yCenter) * Scale + yCenter + cell.speedY;
			//x2 = (x2 - xCenter) * Scale + xCenter + cell.speedX;
			//y2 = (y2 - yCenter) * Scale + yCenter + cell.speedY;




			// Сначала расширяем
			float xCenter = x1 + 0.5f;
			float yCenter = y1 + 0.5f;
			x1 = (x1 - xCenter) * Scale + xCenter;
			y1 = (y1 - yCenter) * Scale + yCenter;
			x2 = (x2 - xCenter) * Scale + xCenter;
			y2 = (y2 - yCenter) * Scale + yCenter;

			// Корретируем новую позицию, чтобы из-за расширения не зацепить клетки позади по ходу движения
			if (fabs(cell.speedX) > 0.001f || fabs(cell.speedY) > 0.001f) {
				float delta = (float)x - x1; // На сколько увеличенная клетка вылазит за пределы старой клетки (с одной стороны)
				float comX = 0;
				float comY = 0;
				if (fabs(cell.speedX) < fabs(cell.speedY)) {
					float k = fabs(cell.speedX / cell.speedY);
					comY = cell.speedY < 0 ? -delta : delta;
					comX = cell.speedX < 0 ? -delta * k : delta * k;
				}
				else {
					float k = fabs(cell.speedY / cell.speedX);
					comX = cell.speedX < 0 ? -delta : delta;
					comY = cell.speedY < 0 ? -delta * k : delta * k;
				}
				x1 += comX;
				x2 += comX;
				y1 += comY;
				y2 += comY;
			}

			// Сдвиг клиенти по вектору движения
			x1 += cell.speedX;
			y1 += cell.speedY;
			x2 += cell.speedX;
			y2 += cell.speedY;





			// Расчитаем какие клетки (и с каким весом) цепляет сдвинутая и расширенная текущая клетка
			int x1i = ((int)(x1 + 1000.)) - 1000;
			int y1i = ((int)(y1 + 1000.)) - 1000;
			int x2i = ((int)(x2 + 1000.)) - 1000;
			int y2i = ((int)(y2 + 1000.)) - 1000;

			coeffs.clear();
			float coeffsSum = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					float outPartL = 0;   // Внешняя доля зацепленной клетки по-горизонтали слева
					if (xi == x1i) {
						outPartL = x1 - (float)x1i;
					}
					float outPartR = 0;
					if (xi == x2i) {     // Внешняя доля зацепленной клетки по-горизонтали справа
						outPartR = 1 - (x2 - (float)x2i);
					}
					float partHor = 1 - (outPartL + outPartR);  // Внутренняя доля зацепленной клетки по-горизонтали

					float outPartU = 0;   // Внешняя доля зацепленной клетки по-горизонтали слева
					if (yi == y1i) {
						outPartU = y1 - (float)y1i;
					}
					float outPartD = 0;
					if (yi == y2i) {     // Внешняя доля зацепленной клетки по-горизонтали справа
						outPartD = 1 - (y2 - (float)y2i);
					}
					float partVert = 1 - (outPartU + outPartD);  // Внутренняя доля зацепленной клетки по-горизонтали

					float partAll = partHor * partVert;  // Полная доля зацепленной клетки по-вертикали и по-горизонтали
					coeffsSum += partAll;
					coeffs.push_back(partAll); // Сохраняем для последующей нормализации суммы коэффициентов на 1
				}
			}

			// Нормируем все собранные коэффициенты на 1
			for (float& elem : coeffs) {
				elem /= coeffsSum;
			}

			// Вычитаем плотность дыма и скорости текущей клетки из своей же клетки (аддитивная карта, будет пременена в конце кадра)
			cell.smokeDensAdd -= cell.smokeDens;
			cell.airDensAdd -= cell.airDens;
			cell.speedXAdd -= cell.speedX;
			cell.speedYAdd -= cell.speedY;

			// Прибавляем плотность дыма и скорости текущей клетки ко всем зааффекченным клеткам с нормированными коэффициентами перекрытия по клеткам периметра (аддитивная карта, будет пременена в конце кадра)
			int index = 0;
			for (int yi = y1i; yi <= y2i; ++yi) {
				for (int xi = x1i; xi <= x2i; ++xi) {
					if (xi < 0 || xi >= Scene2DSize || yi < 0 || yi >= Scene2DSize) {
						continue;
					}
					scene2D[xi][yi].smokeDensAdd += cell.smokeDens * coeffs[index];
					scene2D[xi][yi].airDensAdd += cell.airDens * coeffs[index];
					scene2D[xi][yi].speedXAdd += cell.speedX * coeffs[index];
					scene2D[xi][yi].speedYAdd += cell.speedY * coeffs[index];
					index++;
				}
			}
		}
	}
	
	// Применяем карту аддитивки и очищаем её
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			Cell2D& cell = scene2D[x][y];

			cell.smokeDens += cell.smokeDensAdd;
			cell.airDens += cell.airDensAdd;
			cell.speedX += cell.speedXAdd;
			cell.speedY += cell.speedYAdd;
			cell.speedX = (float)(cell.speedX * SpeedSlowdown);
			cell.speedY = (float)(cell.speedY * SpeedSlowdown);

			if (cell.speedX > 1) cell.speedX = 1;
			if (cell.speedX < -1) cell.speedX = -1;
			if (cell.speedY > 1) cell.speedY = 1;
			if (cell.speedY < -1) cell.speedY = -1;

			if (cell.smokeDens < 0) {
				//printf("smokeDens = %f\n", cell.smokeDens);
				cell.smokeDens = 0;
			}
			cell.smokeDensAdd = 0;
			cell.airDensAdd = 0;
			cell.speedXAdd = 0;
			cell.speedYAdd = 0;
		}
	}

	// Меняем поле скорости от давления
	//for (int y = 1; y < Scene2DSize-1; ++y) {
	//	for (int x = 1; x < Scene2DSize - 1; ++x) {
	//		Cell2D& cell = scene2D[x][y];
	//		float dx = 0;
	//		float dy = 0;
	//		caclAirDensGradient(x, y, dx, dy);
	//		cell.speedX += dx * 0.1f;
	//		cell.speedY += dy * 0.1f;
	//	}
	//}

	 //Поворачиваем рандомно немного вектора скорости и меняем их скорость
	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			Cell2D& cell = scene2D[x][y];
			float scalarSpeed = sqrt(cell.speedX*cell.speedX + cell.speedY*cell.speedY);
			int index = (cell.waveTableRandomOffset + int(stepN * randomSpeedRotateSpeed * scalarSpeed * 3)) % WaveTableSize;
			double angle = waveTable[index] * randomSpeedRotateAmpl;

			turnPoint(cell.speedX, cell.speedY, angle, cell.speedX, cell.speedY);
			cell.speedX *= (1 + waveTable[index] * randomSpeedAccelerateAml);
			cell.speedY *= (1 + waveTable[index] * randomSpeedAccelerateAml);
		}
	}

	 // Усредняем поле скоростей, чтобы не накапливались эффекты положительной обратной связи
	for (int y = 1; y < Scene2DSize-1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].avgSpeedX = (scene2D[x][y].speedX + scene2D[x - 1][y].speedX + scene2D[x + 1][y].speedX + scene2D[x][y - 1].speedX + scene2D[x][y + 1].speedX + scene2D[x - 1][y - 1].speedX + scene2D[x + 1][y - 1].speedX + scene2D[x - 1][y + 1].speedX + scene2D[x + 1][y + 1].speedX) / 9;
			scene2D[x][y].avgSpeedY = (scene2D[x][y].speedY + scene2D[x - 1][y].speedY + scene2D[x + 1][y].speedY + scene2D[x][y - 1].speedY + scene2D[x][y + 1].speedY + scene2D[x - 1][y - 1].speedY + scene2D[x + 1][y - 1].speedY + scene2D[x - 1][y + 1].speedY + scene2D[x + 1][y + 1].speedY) / 9;
		}
	}
	for (int y = 1; y < Scene2DSize - 1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].speedX = scene2D[x][y].avgSpeedX;
			scene2D[x][y].speedY = scene2D[x][y].avgSpeedY;
		}
	}


}

//--------------------------------------------------------------------------------------------

void setSourseSmokeDensityAndSpeed2(int frame)
{
	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			Cell2D& cell = scene2D[x][y];
			cell.speedX = 0.5f;
			cell.airDens = 0.1f;
			if (frame < 100) {
				cell.smokeDens = randf(0, 0.6f);
			}
			else {
				cell.smokeDens = 0;
			}
		}
	}

	for (int y = 0; y < 120; ++y) {
		for (int x = 0; x < 10; ++x) {
			Cell2D& cell = scene2D[Scene2DSize - 1 - x][Scene2DSize - 1 - y];
			cell.speedX = -0.5f;
			cell.airDens = 0.1f;

			if (frame < 100) {
				if (randf(0.f, 1.f) < 0.5)
					cell.smokeDens = 0;
				else
					cell.smokeDens = 0.6f;
			}
			else {
				cell.smokeDens = 0;
			}
		}
	}
}

//--------------------------------------------------------------------------------------------

void test2Render2dAnimation()
{
	Scale = 1.02f; // 1.005f; // 1.002f; // 1.5f;
	SpeedSlowdown = 0.999f;
	randomSpeedRotateAmpl = 0.05f;
	randomSpeedRotateSpeed = 0.3f;
	randomSpeedAccelerateAml = 0;

	initWaveTable();

	//for (int y = 0; y < Scene2DSize; ++y) {
	//	for (int x = 0; x < Scene2DSize; ++x) {
	//		scene2D[x][y].airDens = 0.1f;
	//	}
	//}

	for (int y = 0; y < Scene2DSize; ++y) {
		for (int x = 0; x < Scene2DSize; ++x) {
			scene2D[x][y].waveTableRandomOffset = rand(0, 999);
		}
	}

	// Разблурить рандомные оффсеты 
	for (int y = 1; y < Scene2DSize - 1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].avgWaveTableRandomOffset = (scene2D[x][y].waveTableRandomOffset + scene2D[x - 1][y].waveTableRandomOffset + scene2D[x + 1][y].waveTableRandomOffset + scene2D[x][y - 1].waveTableRandomOffset + scene2D[x][y + 1].waveTableRandomOffset + scene2D[x - 1][y - 1].waveTableRandomOffset + scene2D[x + 1][y - 1].waveTableRandomOffset + scene2D[x - 1][y + 1].waveTableRandomOffset + scene2D[x + 1][y + 1].waveTableRandomOffset) / 9;
		}
	}
	for (int y = 1; y < Scene2DSize - 1; ++y) {
		for (int x = 1; x < Scene2DSize - 1; ++x) {
			scene2D[x][y].waveTableRandomOffset = scene2D[x][y].avgWaveTableRandomOffset;
		}
	}


	for (int i = 0; i < 1000; ++i) {
		setSourseSmokeDensityAndSpeed2(i);
		update2dScene(i);
		if (i % 1 == 0) {
			char number[4];
			number[0] = i / 100 + '0';
			number[1] = (i % 100) / 10 + '0';
			number[2] = (i % 10) + '0';
			number[3] = 0;
			std::string fname = std::string("Animation2d/frame") + (const char*)number + ".bmp";
			save2DSceneToBmp(fname);
		}
		printf("i=%d ", i);
	}
}



	{
		std::shared_ptr<NodeBase> pLeaf = std::make_shared<NodeLeaf>();
		pLeaf->id = 100;

		std::shared_ptr<NodeBranch> pBr101 = std::make_shared<NodeBranch>();
		pBr101->id = 101;
		pBr101->childNodes.push_back(NodeRef(0, 0, 0, 0, pLeaf));

		std::shared_ptr<NodeBranch> pBr102 = std::make_shared<NodeBranch>();
		pBr102->id = 102;
		pBr102->childNodes.push_back(NodeRef(0, 0, 0, 0, pLeaf));

		std::shared_ptr<NodeBranch> pBr103 = std::make_shared<NodeBranch>();
		pBr103->id = 103;
		pBr103->childNodes.push_back(NodeRef(0, 0, 0, 0, std::shared_ptr<NodeBase>(pBr101)));
		pBr103->childNodes.push_back(NodeRef(0, 0, 0, 0, std::shared_ptr<NodeBase>(pBr102)));

		std::shared_ptr<NodeBranch> pBr104 = std::make_shared<NodeBranch>();
		pBr104->id = 104;
		pBr104->childNodes.push_back(NodeRef(0, 0, 0, 0, std::shared_ptr<NodeBase>(pBr101)));
		pBr104->childNodes.push_back(NodeRef(0, 0, 0, 0, std::shared_ptr<NodeBase>(pBr102)));

		object.id = 105;
		object.childNodes.push_back(NodeRef(0, 0, 0, 0, std::shared_ptr<NodeBase>(pBr103)));
		object.childNodes.push_back(NodeRef(0, 0, 0, 0, std::shared_ptr<NodeBase>(pBr104)));

		object.serialize("Test.bin");
		exit_msg("Test done");
	}









