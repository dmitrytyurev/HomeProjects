

//===============================================================================
//
//===============================================================================

void SClientMessagesMgr::test()
{
	// Заполнение тестовых данных
	std::vector<TextTranslated> cltTexts;  // "Как бы" клиентские тексты

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id0";
	cltTexts.back().timestampModified = 11746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id1";
	cltTexts.back().timestampModified = 12746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id2";             // Это ключ 0
	cltTexts.back().timestampModified = 23746908;     //

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id3";
	cltTexts.back().timestampModified = 33746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id4";
	cltTexts.back().timestampModified = 43746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id5";
	cltTexts.back().timestampModified = 53746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id6";            // Это ключ 1
	cltTexts.back().timestampModified = 63746908;    // 

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id7";
	cltTexts.back().timestampModified = 73746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id8";
	cltTexts.back().timestampModified = 83746908;

	cltTexts.emplace_back();
	cltTexts.back().id = "test_text_id9";
	cltTexts.back().timestampModified = 93746908;


	std::vector<std::vector<uint8_t>> textKeys;
	for (auto& text : cltTexts) {
		textKeys.emplace_back();
		MakeKey(text.timestampModified, text.id, textKeys.back());
	}

	ClientFolder cltFolder;  // Для теста нужны: keys, intervals

	cltFolder.keys.emplace_back(textKeys[2]);
	cltFolder.keys.emplace_back(textKeys[6]);

	uint64_t hash = 0;
	hash = AddHash(hash, textKeys[0]);
	hash = AddHash(hash, textKeys[1]);
	cltFolder.intervals.emplace_back(2, hash);

	hash = 0;
	hash = AddHash(hash, textKeys[2]);
	hash = AddHash(hash, textKeys[3]);
	hash = AddHash(hash, textKeys[4]);
	hash = AddHash(hash, textKeys[5]);
	cltFolder.intervals.emplace_back(4, hash);

	hash = 0;
	hash = AddHash(hash, textKeys[6]);
	hash = AddHash(hash, textKeys[7]);
	hash = AddHash(hash, textKeys[8]);
	hash = AddHash(hash, textKeys[9]);
	cltFolder.intervals.emplace_back(4, hash);

	Folder srvFolder;  // Для теста нужны: тексты. Из текстов нужны timestampModified, id

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id0";
	srvFolder.texts.back()->timestampModified = 11746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id1";
	srvFolder.texts.back()->timestampModified = 12746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id2";
	srvFolder.texts.back()->timestampModified = 23746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id3";
	srvFolder.texts.back()->timestampModified = 33746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id4";
	srvFolder.texts.back()->timestampModified = 43746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id5";
	srvFolder.texts.back()->timestampModified = 53746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id6";
	srvFolder.texts.back()->timestampModified = 63746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id7";
	srvFolder.texts.back()->timestampModified = 73746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id8";
	srvFolder.texts.back()->timestampModified = 83746908;

	srvFolder.texts.emplace_back(new TextTranslated);
	srvFolder.texts.back()->id = "test_text_id9";
	srvFolder.texts.back()->timestampModified = 93746908;


	// Начинаем тест
	auto buffer = std::make_shared<SerializationBuffer>();
	auto srvFoldrItr = &srvFolder;
	auto cltFoldrItr = &cltFolder;

	// Заполняем ключи серверных текстов и ссылки на них для быстрой сортировки по ключам

	std::vector<TextKey> textsKeys;       // Ключи серверных текстов текущей папки (для сортировки и разбиения на интевалы)
	std::vector<TextKey*> textsKeysRefs;  // Указатели на эти ключи для быстрой сортировки

	textsKeys.reserve(srvFoldrItr->texts.size());
	textsKeysRefs.reserve(srvFoldrItr->texts.size());

	for (const auto& t : srvFoldrItr->texts) {
		textsKeys.emplace_back();
		auto& textKey = textsKeys.back();
		textKey.textRef = &*t;
		textsKeysRefs.emplace_back(&textKey);
		MakeKey(t->timestampModified, t->id, textKey.key); // Склеивает ts модификации текста и текстовый айдишник текста в "ключ"
	}

	// Сортируем ссылки на заполненные ключи серверных текстов
	std::sort(textsKeysRefs.begin(), textsKeysRefs.end(), [](TextKey* el1, TextKey* el2) { return IfKeyALess(&el1->key[0], el1->key.size(), &el2->key[0], el2->key.size()); });

	//for (auto el : textsKeysRefs) {
	//	printf("%s\n", el->key.data()+4);
	//}
	//return;


	// Заполнение интервалов отсортированных серверных текстов текущей папки
	std::vector<Interval> intervals;
	intervals.resize(cltFoldrItr->intervals.size());

	int cltKeyIdx = 0;
	int sum = 0;

	for (int i = 0; i < (int)textsKeysRefs.size(); ++i) {
		while (!IfKeyALess(&textsKeysRefs[i]->key[0], textsKeysRefs[i]->key.size(), &cltFoldrItr->keys[cltKeyIdx][0], cltFoldrItr->keys[cltKeyIdx].size())) {
			sum += intervals[cltKeyIdx].textsNum;
			++cltKeyIdx;
			intervals[cltKeyIdx].firstTextIdx = sum;
			if (cltKeyIdx == cltFoldrItr->keys.size()) {
				intervals[cltKeyIdx].textsNum = textsKeysRefs.size() - i;
				goto out;
			}
		}
		++(intervals[cltKeyIdx].textsNum);
	}
out:    // Заполняем значения хэшей в интервалах (хэш интервала считается, как хэш ключей всех текстов интервала)
	for (auto& interval : intervals) {
		uint64_t hash = 0;
		for (uint32_t i = interval.firstTextIdx; i < interval.firstTextIdx + interval.textsNum; ++i) {
			hash = AddHash(hash, textsKeysRefs[i]->key);
		}
		interval.hash = hash;
	}

	// Сравниваем интервалы текущего каталога у сервера и клиента. Если текущий интервал отличается, то для него будут посланы все тексты

	uint32_t intervalsDifferNum = 0;  // Подсчитать количество отличающихся интервалов
	for (int i = 0; i < (int)intervals.size(); ++i) {
		if (intervals[i].textsNum != cltFoldrItr->intervals[i].textsInIntervalNum ||
			intervals[i].hash != cltFoldrItr->intervals[i].hashOfKeys) {
			++intervalsDifferNum;
		}
	}
	buffer->Push(intervalsDifferNum);
	for (uint32_t i = 0; i < intervals.size(); ++i) {
		if (intervals[i].textsNum != cltFoldrItr->intervals[i].textsInIntervalNum ||
			intervals[i].hash != cltFoldrItr->intervals[i].hashOfKeys) {
			buffer->Push(i);
			buffer->Push(intervals[i].textsNum);

			for (uint32_t i2 = intervals[i].firstTextIdx; i2 < intervals[i].firstTextIdx + intervals[i].textsNum; ++i2) {
				textsKeysRefs[i2]->textRef->SaveToBase(*buffer);
			}
		}
	}
}
