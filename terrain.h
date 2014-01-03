//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: terrain.h
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Represents a 3D terrain.       
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __terrainH__
#define __terrainH__

#include "d3dUtility.h"
#include <string>
#include <vector>
	
struct Ray
	{
		D3DXVECTOR3 _origin;
		D3DXVECTOR3 _direction;
	};
struct TriangleInfo
{
	int first;
    int second;
	int third;
};
struct VerticesInfo
{
	float x;
	float y;
	float z;
};
class Terrain
{
public:
	VerticesInfo *vInfo; //存储点有关的信息
	TriangleInfo *tInfo; //三角形有关的信息

	Terrain(
		IDirect3DDevice9* device,
		std::string heightmapFileNames[], 
		int numVertsPerRow,  
		int numVertsPerCol, 
		int cellSpacing,    // space between cells
		float heightScale);   

	~Terrain();

	int  getHeightmapEntry(int row, int col);
	void setHeightmapEntry(int row, int col, int value);

	float getHeight(float x, float z);

	bool  loadTexture(std::string fileName);
	bool  genTexture(D3DXVECTOR3* directionToLight);
	bool  draw(D3DXMATRIX* world, bool drawTris);
	int	  getTriangleNumber();
	int   pickTriangle(HWND hwnd);
	bool  changeHeight(int triangleIndex);
	bool  averageHeight(int triangleIndex);

	VerticesInfo getVerticesInfo(int index);
	TriangleInfo getTriangleInfo(int index);
private:
	IDirect3DDevice9*       _device;
	IDirect3DTexture9*      _tex;
	IDirect3DVertexBuffer9* _vb;
	IDirect3DIndexBuffer9*  _ib;
	int _numVertsPerRow;
	int _numVertsPerCol;
	int _cellSpacing;

	int _numCellsPerRow;
	int _numCellsPerCol;
	int _width;
	int _depth;
	int _numVertices;
	int _numTriangles;

	float _heightScale;

	std::vector<int> _heightmap; //存储地形点用的东西

	// helper methods
	bool  readRawFile(std::string fileNames[]);
	bool  computeVertices();
	bool  computeIndices();
	bool  computeMeshVertices();
	bool  computeMeshIndices();
	bool  lightTerrain(D3DXVECTOR3* directionToLight);
	float computeShade(int cellRow, int cellCol, D3DXVECTOR3* directionToLight);

	struct TerrainVertex
	{
		TerrainVertex(){}
		TerrainVertex(float x, float y, float z, float u, float v)
		{
			_x = x; _y = y; _z = z; _u = u; _v = v;
		}
		float _x, _y, _z;
		float _u, _v;

		static const DWORD FVF;
	};
};

#endif // __terrainH__
