#pragma once
#include "Model.h"

class Cube : public Model

{
	unsigned numberOfIndices = 0;

public:
		Cube(ID3D11Device* dxdevice, ID3D11DeviceContext* dxdevice_context);

		virtual void Render() const;

		~Cube() {}
};