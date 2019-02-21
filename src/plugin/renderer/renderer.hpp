#pragma once

namespace wr
{
	class CPUTextures;
	
}

namespace wmr
{

	class ModelManager;
	class TextureManager;
	class FramegraphManager;

	class renderer
	{
	public:
		renderer();
		~renderer();
		void Initialize();
		void Update();
		void Render();

	private:

	};
}
