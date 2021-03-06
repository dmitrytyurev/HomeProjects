#pragma once
#include <vector>
#include <stdint.h>
#include <functional>
#include "../SharedSrc/DeserializationBuffer.h"
#include "../SharedSrc/SerializationBuffer.h"

class SHttpManagerLowImpl;

//===============================================================================
//
//===============================================================================
class SHttpManagerLow // PIMPL ���������� �� ������� � ����� SHttpManager ���������-��������� ��������� �������������
{
public:
	SHttpManagerLow(std::function<void(DeserializationBuffer&, SerializationBuffer&)> requestCallback); // ���������� �������, ������� ����� ������ ��� ������ �������� http-������� (���������� �����!). ������� ������ ������������ ����� �� ������
	~SHttpManagerLow();
	void Update(double dt);


private:
	void StartHttpListening();

private:
	std::unique_ptr<SHttpManagerLowImpl> _pImpl;
};