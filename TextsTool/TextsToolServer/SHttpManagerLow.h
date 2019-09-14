#pragma once
#include <vector>
#include <stdint.h>
#include <functional>

class SHttpManagerLowImpl;

//===============================================================================
//
//===============================================================================
class SHttpManagerLow // PIMPL ���������� �� ������� � ����� SHttpManager ���������-��������� ��������� �������������
{
public:
	SHttpManagerLow(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback); // ���������� �������, ������� ����� ������ ��� ������ �������� http-������� (���������� �����!). ������� ������ ������������ ����� �� ������
	void StartHttpListening();
	~SHttpManagerLow();
	void Update(double dt);

private:
	std::unique_ptr<SHttpManagerLowImpl> _pImpl;
};