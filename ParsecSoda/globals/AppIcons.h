#pragma once

#include "../Texture.h"

typedef ID3D11ShaderResourceView* Icon;

class AppIcons
{
public:
	static void init(ID3D11Device* pd3dDevice);

	static ID3D11ShaderResourceView* play;
	static ID3D11ShaderResourceView* stop;

	static ID3D11ShaderResourceView* yes;
	static ID3D11ShaderResourceView* no;

	static ID3D11ShaderResourceView* send;
	static ID3D11ShaderResourceView* back;

	static ID3D11ShaderResourceView* lock;
	static ID3D11ShaderResourceView* unlock;

	static ID3D11ShaderResourceView* padon;
	static ID3D11ShaderResourceView* padoff;
	static ID3D11ShaderResourceView* plug;

	static ID3D11ShaderResourceView* mic;
	static ID3D11ShaderResourceView* speakers;

	static ID3D11ShaderResourceView* refresh;
	static ID3D11ShaderResourceView* submit;

private:
	static Texture _play;
	static Texture _stop;

	static Texture _yes;
	static Texture _no;
	
	static Texture _send;
	static Texture _back;

	static Texture _padon;
	static Texture _padoff;
	static Texture _plug;

	static Texture _lock;
	static Texture _unlock;
	
	static Texture _mic;
	static Texture _speakers;
	
	static Texture _refresh;
	static Texture _submit;
};
